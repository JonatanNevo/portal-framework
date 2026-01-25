//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_registry.h"

#include <ranges>

#include "portal/engine/modules/system_orchestrator.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/resources/source/file_source.h"

namespace portal
{
static auto logger = Log::get_logger("Resources");

ResourceRegistry::ResourceRegistry(
    const Project& project,
    ecs::Registry& ecs_registry,
    jobs::Scheduler& scheduler,
    ResourceDatabase& database,
    ReferenceManager& reference_manager,
    const renderer::vulkan::VulkanContext& context
) : ecs_registry(ecs_registry),
    scheduler(scheduler),
    database(database),
    reference_manager(reference_manager),
    loader_factory(
        project,
        *this,
        context
    )
{}

ResourceRegistry::~ResourceRegistry() noexcept
{
    std::unordered_map<StringId, WeakReference<Resource>> weak_resources;
    weak_resources.reserve(resources.size());
    for (auto& [resource_name, ref] : resources)
        weak_resources[resource_name] = ref.resource;

    resources.clear();

    for (auto& [resource_name, ref] : weak_resources)
    {
        auto resource = ref.lock();
        if (resource)
        {
            LOGGER_ERROR("Dangling resource: {} of type: {}", resource_name, resource->static_type());
        }
    }
}

void ResourceRegistry::save(const StringId& resource_id)
{
    {
        std::lock_guard guard(lock);
        if (resources.contains(resource_id) || pending_resources.contains(resource_id))
            return;
    }
    save_resource(resources.at(resource_id));
}

Job<resources::ResourceData> ResourceRegistry::load_direct(SourceMetadata meta, const Reference<resources::ResourceSource> source)
{
    // TODO: add check that the resource does not exist already?
    LOGGER_TRACE("Creating resource: {} of type: {}", meta.resource_id, meta.type);

    auto& loader = loader_factory.get(meta);

    // TODO: have load as a coroutine as well?
    auto resource_data = loader.load(meta, source);
    if (!resource_data.resource)
    {
        LOGGER_ERROR("Failed to load resource: {}", meta.resource_id);
        co_return {};
    }

    std::lock_guard guard(lock);
    resources[meta.resource_id] = resource_data;
    co_return resource_data;
}

void ResourceRegistry::wait_all(const std::span<Job<>> jobs) const
{
    scheduler.wait_for_jobs(jobs);
}

void ResourceRegistry::save_resource(resources::ResourceData& resource_data)
{
    {
        std::lock_guard guard(lock);
        if (resource_data.dirty == ResourceDirtyBits::Clean)
            return;
    }

    auto& loader = loader_factory.get(resource_data.metadata);
    loader.save(resource_data);

    {
        std::lock_guard guard(lock);
        resource_data.dirty = ResourceDirtyBits::Clean;
    }
}

std::expected<Reference<Resource>, ResourceState> ResourceRegistry::get_resource(const StringId& id)
{
    std::lock_guard guard(lock);
    if (resources.contains(id))
        return resources.at(id).resource;

    if (pending_resources.contains(id))
        return std::unexpected{ResourceState::Pending};

    if (errored_resources.contains(id))
        return std::unexpected{ResourceState::Error};

    if (database.find(id).has_value())
        return std::unexpected{ResourceState::Unloaded};

    LOG_ERROR("Attempted to get resource with handle {} that does not exist", id);
    return std::unexpected{ResourceState::Missing};
}

void ResourceRegistry::create_resource(const StringId& resource_id, [[maybe_unused]] ResourceType type)
{
    {
        std::lock_guard guard(lock);
        if (resources.contains(resource_id) || pending_resources.contains(resource_id))
            return;
    }

    scheduler.dispatch_job(load_resource(resource_id));
}

void ResourceRegistry::create_resource_immediate(const StringId& resource_id)
{
    {
        std::lock_guard guard(lock);
        if (resources.contains(resource_id) || pending_resources.contains(resource_id))
            return;
    }

    scheduler.wait_for_job(load_resource(resource_id));
}

Job<Reference<Resource>> ResourceRegistry::load_resource(const StringId resource_id)
{
    if (pending_resources.contains(resource_id) || resources.contains(resource_id))
        co_return nullptr;

    {
        std::lock_guard guard(lock);
        pending_resources.insert(resource_id);
    }

    const auto meta_result = database.find(resource_id);
    if (!meta_result)
    {
        std::lock_guard guard(lock);
        LOGGER_ERROR("Failed to find metadata for resource with id: {}", resource_id);
        errored_resources.insert(resource_id);
        pending_resources.erase(resource_id);
        co_return nullptr;
    }

    SourceMetadata meta = *meta_result;
    // TODO these checks should be done in the database
    if (meta.source.string.starts_with("composite://"))
    {
        auto source_view = std::string_view(meta_result->source.string);
        source_view.remove_prefix(std::strlen("composite://"));

        const auto composite_id = STRING_ID(source_view.substr(0, source_view.find("/gltf")));
        meta = database.find(composite_id).value();
    }

    const auto source = database.create_source(meta.resource_id, meta);
    auto job = load_direct(meta, source);
    co_await job;
    auto resource_data = job.result();
    if (!resource_data || resource_data.value().resource == nullptr)
    {
        std::lock_guard guard(lock);
        errored_resources.insert(meta.resource_id);
        pending_resources.erase(meta.resource_id);
        co_return nullptr;
    }

    std::lock_guard guard(lock);
    pending_resources.erase(resource_id);
    co_return resource_data.value().resource;
}

void ResourceRegistry::set_dirty(const StringId& resource_id, const ResourceDirtyFlags flags)
{
    std::lock_guard guard(lock);
    resources[resource_id].dirty |= flags;
}

ResourceDirtyFlags ResourceRegistry::get_dirty(const StringId& resource_id)
{
    std::lock_guard guard(lock);
    return resources[resource_id].dirty;
}
} // portal
