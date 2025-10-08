//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_registry.h"

#include <ranges>

#include "database/resource_database.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/resources/loader/loader.h"
#include "resources/resource.h"
#include "source/resource_source.h"
#include "portal/core/thread.h"

namespace portal
{

static auto logger = Log::get_logger("Resources");

Ref<Resource> ResourceRegistry::load(StringId id, const ResourceType type)
{
    PORTAL_PROF_ZONE();
    if (resource_map[type].contains(id))
        return resource_map[type].at(id);

    const auto source = resource_database->get_source(id);
    const auto meta = source->get_meta();

    PORTAL_ASSERT(meta.resource_type == ResourceType::Composite || meta.resource_type == type, "Request resource does not match source");

    LOGGER_TRACE("Requesting load for resource: {}", id);
    asset_load_queue.enqueue({id, source});

    return create_resource_ref({.id = id, .type = type});
}

Ref<Resource> ResourceRegistry::immediate_load(const StringId& id, const ResourceType type)
{
    PORTAL_PROF_ZONE();
    if (resource_map[type].contains(id))
        return resource_map[type].at(id);

    const auto source = resource_database->get_source(id);
    const auto meta = source->get_meta();

    PORTAL_ASSERT(meta.resource_type == ResourceType::Composite || meta.resource_type == type, "Request resource does not match source");

    load_resource(id, source);

    return resource_map[type][id];
}

Ref<Resource> ResourceRegistry::immediate_load(const StringId& id, const std::shared_ptr<resources::ResourceSource>& source)
{
    const auto meta = source->get_meta();
    load_resource(id, source);

    return resource_map[meta.resource_type][meta.source_id];
}

Ref<Resource>& ResourceRegistry::get(const StringId& id, const ResourceType type)
{
    return create_resource_ref({id, type});
}

void ResourceRegistry::unload(ResourceSignature signature)
{
    PORTAL_PROF_ZONE();
    LOGGER_TRACE("Unloading resource: {}", signature.id);
    const auto it = resource_map[signature.type].find(signature.id);
    if (it == resource_map[signature.type].end())
    {
        LOGGER_WARN("Attempted to unload resource {} that is not loaded.", signature.id);
        return;
    }

    // There will always be one reference in the map, so a single reference means that the resource can be unloaded.
    if (it->second->get_ref() > 1)
    {
        LOGGER_WARN("Resource {} has references, not unloading. Ref count: {}", signature.id, it->second->get_ref());
        return;
    }

    // TODO: add to clean queue if 0 instead?
    resource_map[signature.type].erase(it);
}

void ResourceRegistry::set_default(const ResourceType type, const std::optional<Ref<Resource>>& resource)
{
    default_resources[type] = resource;
    if (resource.has_value())
        LOGGER_DEBUG("Setting {} as default for resources of type {}", resource.value()->id, type);
    else
        LOGGER_DEBUG("Setting default resource of type {} to be loader dependent", type);
}

void ResourceRegistry::initialize(const std::shared_ptr<renderer::vulkan::GpuContext>& context, const std::shared_ptr<resources::ResourceDatabase>& database)
{
    LOGGER_INFO("Resource registry initialization...");
    gpu_context = context;
    resource_database = database;
    loader_factory.initialize(this, gpu_context);
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
        asset_loader_thread.join();

    gpu_context = nullptr;

    loader_factory.shutdown();
    default_resources.clear();
    resource_map.clear();
}

Ref<Resource>& ResourceRegistry::create_resource_ref(const ResourceSignature& signature)
{
    std::lock_guard lock(resource_lock);
    auto& resource_ref = resource_map[signature.type][signature.id];
    if (resource_ref == nullptr)
    {
        LOGGER_TRACE("Creating new {} for id: {}", signature.type, signature.id);
        resource_ref = resources::utils::create_resource(signature.id, signature.type);
        load_default(resource_ref, signature.type);
    }
    return resource_ref;
}

void ResourceRegistry::load_default(Ref<Resource>&, ResourceType)
{
    // const auto default_resource = default_resources[type];
    // if (default_resource.has_value())
    // {
    //     resource->copy_from(default_resource.value());
    // }
    // else
    // {
    //     const auto loader = loader_factory.get(type);
    //     loader->load_default(resource);
    // }
}

void ResourceRegistry::resource_load_loop(const std::stop_token& stoken)
{
    PORTAL_PROF_ZONE();
    ResourceRequest request;
    while (!stoken.stop_requested())
    {
        if (asset_load_queue.try_dequeue(request))
        {
            load_resource(request.resource_id, request.source);
        }
        else
        {
            std::this_thread::yield();
        }
    }
}

void ResourceRegistry::load_resource(const StringId& resource_id, const std::shared_ptr<resources::ResourceSource>& source)
{
    PORTAL_PROF_ZONE();
    const auto meta = source->get_meta();
    const auto& loader = loader_factory.get(meta.resource_type);
    auto ref = get(resource_id, meta.resource_type);

    LOGGER_TRACE("Loading resource: {}", ref->id);

    if (ref->get_state() == ResourceState::Loaded)
    {
        LOGGER_WARN("Resource {} is already loaded, skipping load request.", ref->id);
        return;
    }

    const auto res = loader->load(resource_id, source);
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
} // portal
