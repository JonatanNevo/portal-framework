//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_registry.h"

#include <ranges>

#include "portal/engine/renderer/renderer_context.h"

namespace portal
{

static auto logger = Log::get_logger("Resources");

ResourceRegistry::ResourceRegistry(
    ReferenceManager& ref_manager,
    ResourceDatabase& database,
    jobs::Scheduler& scheduler,
    RendererContext& context
    )
    : database(database),
      reference_manager(ref_manager),
      scheduler(scheduler),
      context(context),
      loader_factory(*this, context) // TODO: this might not work :(
{

}

Reference<Resource> ResourceRegistry::load_direct(const SourceMetadata& meta, const resources::ResourceSource& source)
{
    // TODO: add check that the resource does not exist already?
    LOGGER_TRACE("Creating resource: {} of type: {}", meta.resource_id, meta.type);

    auto& loader = loader_factory.get(meta);

    // TODO: have load as a coroutine as well?
    auto resource = loader.load(meta, source);
    if (!resource)
    {
        LOGGER_ERROR("Failed to load resource: {}", meta.resource_id);
        return nullptr;
    }

    return resource;
}

std::expected<Reference<Resource>, ResourceState> ResourceRegistry::get_resource(const ResourceHandle handle) const
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

ResourceHandle ResourceRegistry::create_resource_immediate(const StringId& resource_id, [[maybe_unused]] ResourceType type)
{
    const auto handle = to_resource_handle(resource_id);

    if (resources.contains(handle) || pending_resources.contains(handle))
        return handle;

    scheduler.wait_for_job(load_resource(handle));
    return handle;
}

ResourceHandle ResourceRegistry::to_resource_handle(const StringId& resource_id)
{
    return resource_id.id;
}

Job<Reference<Resource>> ResourceRegistry::load_resource(const ResourceHandle handle)
{
    // TODO: synchronization for maps and sets?

    pending_resources.insert(handle);

    const auto meta = database.find(handle);
    if (!meta)
    {
        LOGGER_ERROR("Failed to find metadata for resource with handle: {}", handle);
        errored_resources.insert(handle);
        pending_resources.erase(handle);
        co_return nullptr;
    }

    const auto source = database.create_source(*meta);

    auto resource = load_direct(*meta, *source);
    if (!resource)
    {
        errored_resources.insert(handle);
        pending_resources.erase(handle);
        co_return nullptr;
    }
    resources[handle] = resource;
    pending_resources.erase(handle);

    co_return resource;
}

} // portal
