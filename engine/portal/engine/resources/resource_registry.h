//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <thread>
#include <concurrentqueue/concurrentqueue.h>

#include "portal/core/concurrency/spin_lock.h"
#include "../renderer/vulkan/gpu_context.h"
#include "portal/engine/resources/loader/loader_factory.h"
#include "portal/engine/strings/string_id.h"
#include "utils.h"
#include "portal/core/reference.h"
#include "portal/core/thread.h"
#include "portal/core/concurrency/reentrant_spin_lock.h"
#include "portal/core/debug/profile.h"

namespace portal
{
namespace resources
{
    class ResourceDatabase;
}

class Resource;

class ResourceRegistry
{
public:
    /**
     * Initialize the resource registry with GPU context and resource database
     * @param context The GPU context used for resource loading operations
     * @param database The resource database containing metadata and source information for resources
     */
    void initialize(const std::shared_ptr<renderer::vulkan::GpuContext>& context, const std::shared_ptr<resources::ResourceDatabase>& database);

    /**
     * Shutdown and cleanup the resource registry.
     * This will unload all resources and stop the asset loading thread.
     */
    void shutdown();

    /**
     * Request an asynchronous load for a resource of type T.
     * The resource will be loaded in a background thread.
     *
     * @tparam T The resource type to load, must inherit from Resource
     * @param id The unique identifier of the resource to load
     * @return A reference to the resource. The resource may not be fully loaded when returned,
     *         check resource state with is_valid() before use
     */
    template <typename T> requires std::is_base_of_v<Resource, T>
    Ref<T> load(const StringId id)
    {
        auto resource_type = resources::utils::to_resource_type<T>();
        auto&& resource = load(id, resource_type);
        return std::move(resource.template as<T>());
    }

    /**
     * Request an asynchronous load for a resource.
     * The resource will be loaded in a background thread.
     *
     * @param id The unique identifier of the resource to load
     * @param type The type of resource to load
     * @return A reference to the resource. The resource may not be fully loaded when returned,
     *         check resource state with is_valid() before use
     */
    Ref<Resource> load(StringId id, ResourceType type);

    /**
     * Synchronously load a resource of type T.
     * This function blocks until the resource is fully loaded.
     * 
     * @tparam T The resource type to load, must inherit from Resource
     * @param id The unique identifier of the resource to load
     * @return A reference to the fully loaded resource
     */
    template <typename T> requires std::is_base_of_v<Resource, T>
    Ref<T> immediate_load(const StringId id)
    {
        auto resource_type = resources::utils::to_resource_type<T>();
        auto&& resource = immediate_load(id, resource_type);
        return std::move(resource.template as<T>());
    }

    /**
     * Synchronously load a resource.
     * This function blocks until the resource is fully loaded.
     *
     * @param id The unique identifier of the resource to load
     * @param type The type of resource to load
     * @return A reference to the fully loaded resource
     */
    Ref<Resource> immediate_load(const StringId& id, ResourceType type);

    /**
     * Synchronously load a resource from a source.
     * This function blocks until the resource is fully loaded.
     *
     * @param id The unique identifier of the resource to load
     * @param source The resource source containing load data
     * @return A reference to the fully loaded resource
     */
    Ref<Resource> immediate_load(const StringId& id, const std::shared_ptr<resources::ResourceSource>& source);

    /**
     * Get a reference to an existing resource of type T.
     * If the resource doesn't exist, creates an empty resource reference.
     * Does not trigger resource loading.
     * 
     * @tparam T The resource type to get, must inherit from Resource
     * @param id The unique identifier of the resource
     * @return A reference to the resource, may be empty if not loaded
     */
    template <typename T> requires std::is_base_of_v<Resource, T>
    Ref<T> get(const StringId id)
    {
        auto resource_type = resources::utils::to_resource_type<T>();
        auto&& resource = get(id, resource_type);
        return std::move(resource.template as<T>());
    }

    /**
     * Get a reference to an existing resource.
     * If the resource doesn't exist, creates an empty resource reference.
     * Does not trigger resource loading.
     *
     * @param id The unique identifier of the resource
     * @param type The type of resource
     * @return A reference to the resource, may be empty if not loaded
     */
    Ref<Resource>& get(const StringId& id, ResourceType type);

    /**
     * Attempt to unload a resource from memory.
     * The resource will only be unloaded if there are no remaining references to it.
     * 
     * @param signature The unique signature identifying the resource to unload
     */
    void unload(ResourceSignature signature);

    void set_default(ResourceType type, const std::optional<Ref<Resource>>& resource);

protected:
    struct ResourceRequest
    {
        StringId resource_id;
        std::shared_ptr<resources::ResourceSource> source;
    };
    using ResourceMap = std::unordered_map<StringId, Ref<Resource>>;


    void resource_load_loop(const std::stop_token& stoken);
    void load_resource(const StringId& resource_id, const std::shared_ptr<resources::ResourceSource>& source);

    Ref<Resource>& create_resource_ref(const ResourceSignature& signature);
    void load_default(Ref<Resource>& resource, ResourceType type);

private:
    std::shared_ptr<renderer::vulkan::GpuContext> gpu_context;
    std::unordered_map<ResourceType, ResourceMap> resource_map;
    std::unordered_map<ResourceType, std::optional<Ref<Resource>>> default_resources;\

    ReentrantSpinLock<> resource_lock;
    Thread asset_loader_thread;
    moodycamel::ConcurrentQueue<ResourceRequest> asset_load_queue;

    std::shared_ptr<resources::ResourceDatabase> resource_database;
    resources::LoaderFactory loader_factory;
};

} // portal
