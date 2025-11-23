//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_registry.h"

#include <ranges>

#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/resources/source/file_source.h"

namespace portal
{
static auto logger = Log::get_logger("Resources");


ResourceRegistry::ResourceRegistry(ModuleStack& stack, const RendererContext& context)
    : Module<ReferenceManager, ResourceDatabase, SchedulerModule>(stack, STRING_ID("Resource Registry")),
      loader_factory(
          *this,
          context
      ) // TODO: this might not work :( {}
{}

ResourceRegistry::~ResourceRegistry() noexcept
{
    std::unordered_map<StringId, WeakReference<Resource>> weak_resources;
    weak_resources.reserve(resources.size());
    for (auto& [name, ref] : resources)
        weak_resources[name] = ref;

    resources.clear();

    for (auto& [name, ref] : weak_resources)
    {
        auto resource = ref.lock();
        if (resource)
        {
            LOGGER_ERROR("Dangling resource: {} of type: {}", name, resource->static_type());
        }
    }
}

Job<Reference<Resource>> ResourceRegistry::load_direct(const SourceMetadata& meta, const resources::ResourceSource& source)
{
    // TODO: add check that the resource does not exist already?
    LOGGER_TRACE("Creating resource: {} of type: {}", meta.resource_id, meta.type);

    auto& loader = loader_factory.get(meta);

    // TODO: have load as a coroutine as well?
    auto resource = loader.load(meta, source);
    if (!resource)
    {
        LOGGER_ERROR("Failed to load resource: {}", meta.resource_id);
        co_return nullptr;
    }

    std::lock_guard guard(lock);
    resources[meta.resource_id] = resource;
    co_return resource;
}

void ResourceRegistry::wait_all(const std::span<Job<>> jobs)
{
    get_dependency<SchedulerModule>().get_scheduler().wait_for_jobs(jobs);
}

std::expected<Reference<Resource>, ResourceState> ResourceRegistry::get_resource(const StringId& id)
{
    std::lock_guard guard(lock);
    if (resources.contains(id))
        return resources.at(id);

    if (pending_resources.contains(id))
        return std::unexpected{ResourceState::Pending};

    if (errored_resources.contains(id))
        return std::unexpected{ResourceState::Error};

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

    get_dependency<SchedulerModule>().get_scheduler().dispatch_job(load_resource(resource_id));
}

void ResourceRegistry::create_resource_immediate(const StringId& resource_id, [[maybe_unused]] ResourceType type)
{
    {
        std::lock_guard guard(lock);
        if (resources.contains(resource_id) || pending_resources.contains(resource_id))
            return;
    }

    get_dependency<SchedulerModule>().get_scheduler().wait_for_job(load_resource(resource_id));
}

Job<Reference<Resource>> ResourceRegistry::load_resource(const StringId resource_id)
{
    if (pending_resources.contains(resource_id) || resources.contains(resource_id))
        co_return nullptr;

    {
        std::lock_guard guard(lock);
        pending_resources.insert(resource_id);
    }

    const auto meta = get_dependency<ResourceDatabase>().find(resource_id);
    if (!meta)
    {
        std::lock_guard guard(lock);
        LOGGER_ERROR("Failed to find metadata for resource with id: {}", resource_id);
        errored_resources.insert(resource_id);
        pending_resources.erase(resource_id);
        co_return nullptr;
    }

    const auto source = get_dependency<ResourceDatabase>().create_source(*meta);

    auto job = load_direct(*meta, *source);
    co_await job;
    auto resource = job.result();
    if (!resource || resource.value() == nullptr)
    {
        std::lock_guard guard(lock);
        errored_resources.insert(resource_id);
        pending_resources.erase(resource_id);
        co_return nullptr;
    }

    std::lock_guard guard(lock);
    pending_resources.erase(resource_id);
    co_return resource.value();
}
} // portal
