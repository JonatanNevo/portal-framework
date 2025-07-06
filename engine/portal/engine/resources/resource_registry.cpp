//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_registry.h"

#include "database/resource_database.h"
#include "portal/engine/resources/resource.h"
#include "portal/engine/resources/resource_source.h"

namespace portal
{

auto logger = Log::get_logger("Resources");

std::shared_ptr<Resource> ResourceRegistry::get(StringId id)
{
    if (resources.contains(id))
        return resources.at(id).resource;

    auto& resource_ref = resources[id];
    auto* default_resource = database->get_default_resource(id);
    resource_ref.resource = std::shared_ptr<Resource>(default_resource);

    LOGGER_TRACE("Requesting load for resource: {}", id);
    asset_load_queue.enqueue(id);

    resource_ref.increment_ref();
    return resource_ref.resource;
}

void ResourceRegistry::unload(StringId id)
{
    LOGGER_TRACE("Unloading resource: {}", id);
    const auto it = resources.find(id);
    if (it == resources.end())
    {
        LOGGER_WARN("Attempted to unload resource {} that is not loaded.", id);
        return;
    }

    it->second.decrement_ref();
    // TODO: add to clean queue if 0?
}

void ResourceReference::increment_ref()
{
    std::lock_guard guard(lock);
    ref_count++;
}

void ResourceReference::decrement_ref()
{
    std::lock_guard guard(lock);
    ref_count--;
}

void ResourceRegistry::load_assets(const std::stop_token& stoken)
{
    StringId resource_id = {0, ""sv};
    while (!stoken.stop_requested())
    {
        if (asset_load_queue.try_dequeue(resource_id))
        {
            LOGGER_TRACE("Loading resource: {}", resource_id);
            auto& resource_ref = resources[resource_id];
            if (resource_ref.resource->state == ResourceState::Loaded)
            {
                LOGGER_WARN("Resource {} is already loaded, skipping load request.", resource_id);
                continue;
            }

            auto source = database->get_source(resource_id);
            if (const auto source_ptr = source.lock())
            {
                // TODO: Handle allocation
                source_ptr->load({});
                // TODO: Handle resource creation

                resource_ref.resource->state = ResourceState::Loaded;
            }
            else
            {
                LOGGER_ERROR("Resource {} could not be retrieved from the database, marking as missing.", resource_id);
                resource_ref.resource->state = ResourceState::Missing;
            }
        }
        else
        {
            std::this_thread::yield();
        }
    }
}
} // portal
