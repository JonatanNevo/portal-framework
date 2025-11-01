//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_registry.h"

#include <ranges>

namespace portal::ng
{

static auto logger = Log::get_logger("Resources");

ResourceRegistry::ResourceRegistry(
    ReferenceManager& ref_manager,
    ResourceDatabase& database,
    jobs::Scheduler& scheduler,
    renderer::vulkan::VulkanContext& context
    )
    : database(database),
      reference_manager(ref_manager),
      scheduler(scheduler),
      context(context),
      loader_factory(*this, context)// TODO: this might not work :(
{

}

ResourceRegistry::~ResourceRegistry() noexcept
{
    for (auto& [_, resource] : resources)
    {
        delete resource;
    }
}

std::expected<Resource*, ResourceState> ResourceRegistry::get_resource(const ResourceHandle handle) const
{
    if (resources.contains(handle))
        return resources.at(handle);

    if (pending_resources.contains(handle))
        return std::unexpected{ResourceState::Pending};

    if (errored_resources.contains(handle))
        return std::unexpected{ResourceState::Error};

    LOG_ERROR("Attempted to get resource with handle {} that does not exist", handle);
    return std::unexpected{ResourceState::Missing};
}

ResourceHandle ResourceRegistry::create_resource(const StringId& resource_id, [[maybe_unused]] ResourceType type)
{
    const auto handle = to_resource_handle(resource_id);

    if (resources.contains(handle) || pending_resources.contains(handle))
        return handle;

    scheduler.dispatch_job(load_resource(handle));
    return handle;
}

ResourceHandle ResourceRegistry::to_resource_handle(const StringId& resource_id)
{
    return resource_id.id;
}

Job<Resource*> ResourceRegistry::load_resource(const ResourceHandle handle)
{
    // TODO: synchronization for maps and sets?

    pending_resources.insert(handle);

    auto meta = database.find(handle);
    if (!meta)
    {
        LOGGER_ERROR("Failed to find metadata for resource with handle: {}", handle);
        errored_resources.insert(handle);
        pending_resources.erase(handle);
        co_return nullptr;
    }

    auto& loader = loader_factory.get(*meta);
    const auto source = database.create_source(*meta);

    // TODO: have load as a coroutine as well?
    auto* resource = loader.load(*meta, *source);
    if (!resource)
    {
        LOGGER_ERROR("Failed to load resource: {}", meta->resource_id);
        errored_resources.insert(handle);
        pending_resources.erase(handle);
        co_return nullptr;
    }

    resources[handle] = resource;
    pending_resources.erase(handle);

    co_return resource;
}

} // portal
