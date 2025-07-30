//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_registry.h"

#include "database/resource_database.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/resources/loader/loader.h"
#include "resources/resource.h"
#include "source/resource_source.h"
#include "portal/core/thread.h"

namespace portal
{

static auto logger = Log::get_logger("Resources");

Ref<Resource> ResourceRegistry::get(StringId id, ResourceType type)
{
    PORTAL_PROF_ZONE;
    if (resource_map.contains(id))
        return resource_map.at(id);

    auto& resource_ref = resource_map[id];
    if (resource_ref == nullptr)
    {
        LOGGER_TRACE("Creating new {} for id: {}", type, id);
        resource_ref = resources::utils::create_resource(id, type);
    }
    auto source = resource_database->get_source(id);
    const auto meta = source->get_meta();
    const auto& loader = loader_factory.get(meta);
    loader->init(source);
    loader->load_default(resource_ref);

    LOGGER_TRACE("Requesting load for resource: {}", id);
    asset_load_queue.enqueue({id, loader});

    return resource_ref;
}

Ref<Resource> ResourceRegistry::immediate_load(StringId id, ResourceType type)
{
    PORTAL_PROF_ZONE;
    if (resource_map.contains(id))
        return resource_map.at(id);

    auto& resource_ref = resource_map[id];
    if (resource_ref == nullptr)
    {
        LOGGER_TRACE("Creating new {} for id: {}", type, id);
        resource_ref = resources::utils::create_resource(id, type);
    }
    const auto source = resource_database->get_source(id);
    const auto meta = source->get_meta();
    const auto& loader = loader_factory.get(meta);
    loader->init(source);
    load_resource(resource_ref, loader);
    return resource_ref;
}

void ResourceRegistry::unload(StringId id)
{
    PORTAL_PROF_ZONE;
    LOGGER_TRACE("Unloading resource: {}", id);
    const auto it = resource_map.find(id);
    if (it == resource_map.end())
    {
        LOGGER_WARN("Attempted to unload resource {} that is not loaded.", id);
        return;
    }

    // There will always be one reference in the map, so a single reference means that the resource can be unloaded.
    if (it->second->get_ref() > 1)
    {
        LOGGER_WARN("Resource {} has references, not unloading. Ref count: {}", id, it->second->get_ref());
        return;
    }

    // TODO: add to clean queue if 0 instead?
    resource_map.erase(it);
}

void ResourceRegistry::initialize(const std::shared_ptr<resources::GpuContext>& context, const std::shared_ptr<resources::ResourceDatabase>& database)
{
    LOGGER_INFO("Resource registry initialization...");
    gpu_context = context;
    resource_database = database;
    loader_factory.initialize(gpu_context);
    asset_loader_thread = Thread(
        "Asset Loader Thread",
        [&](const std::stop_token& token)
        {
            resource_load_loop(token);
        }
        );
}

void ResourceRegistry::shutdown()
{
    LOGGER_INFO("Shutting down resource registry...");
    asset_loader_thread.request_stop();
    if (asset_loader_thread.joinable())
    {
        asset_loader_thread.join();
    }

    resource_map.clear();
}

void ResourceRegistry::load_resource(Ref<Resource>& ref, const std::shared_ptr<resources::ResourceLoader>& loader)
{
    PORTAL_PROF_ZONE;
    LOGGER_TRACE("Loading resource: {}", ref->id);

    if (ref->get_state() == ResourceState::Loaded)
    {
        LOGGER_WARN("Resource {} is already loaded, skipping load request.", ref->id);
    }

    const auto res = loader->load(ref);
    if (res)
    {
        ref->set_state(ResourceState::Loaded);
    }
    else
    {
        LOGGER_ERROR("Resource {} could not be retrieved from the database, marking as missing.", ref->id);
        ref->set_state(ResourceState::Missing);
    }
}

void ResourceRegistry::resource_load_loop(const std::stop_token& stoken)
{
    PORTAL_PROF_ZONE;
    std::pair<StringId, std::shared_ptr<resources::ResourceLoader>> resource_load_request = {INVALID_STRING_ID, nullptr};
    while (!stoken.stop_requested())
    {
        if (asset_load_queue.try_dequeue(resource_load_request))
        {
            auto& [id, loader] = resource_load_request;
            auto& resource_ref = resource_map[id];
            load_resource(resource_ref, loader);
        }
        else
        {
            std::this_thread::yield();
        }
    }
}
} // portal
