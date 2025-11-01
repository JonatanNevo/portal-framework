//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <portal/core/jobs/scheduler.h>

#include "portal/engine/resources/utils.h"
#include "reference_manager.h"
#include "database/resource_database.h"
#include "loader/loader_factory.h"
#include "portal/engine/reference.h"
#include "portal/engine/resources/resources/resource.h"

namespace portal::renderer::vulkan
{
class VulkanTexture;
}

namespace portal
{

template <ResourceConcept T>
class ResourceReference;

class ResourceRegistry
{
public:
    ResourceRegistry(ReferenceManager& ref_manager, ResourceDatabase& database, jobs::Scheduler& scheduler, RendererContext& context);

    /**
     * Request an asynchronous load for a resource based on its unique id and returns a reference.
     * The returned reference is invalid until the resource is loaded, once its loaded it can be accessed through the `ResourceReference` api
     *
     * @note Resources cannot have an `invalid state` but a reference can have one, make sure to test it before using the underlying resource.
     * @note the resource id is different from the resource handle, while both are unique per resource.
     *
     * @tparam T The underlying requested resource type
     * @param resource_id The resource id
     * @return A reference to the resource
     */
    template <ResourceConcept T>
    ResourceReference<T> load(StringId resource_id)
    {
        auto type = utils::to_resource_type<T>();
        auto handle = create_resource(resource_id, type);

        auto reference = ResourceReference<T>(resource_id, handle, *this, reference_manager);
        return reference;
    }

    /**
     * Request an immediate load of a resource based on its unique id and returns a reference to it.
     *
     * @tparam T The underlying requested resource type
     * @param resource_id The resource id
     * @return A reference to the resource
     */
    template <ResourceConcept T>
    ResourceReference<T> immediate_load(StringId resource_id)
    {
        auto type = utils::to_resource_type<T>();
        auto handle = create_resource_immediate(resource_id, type);

        auto reference = ResourceReference<T>(resource_id, handle, *this, reference_manager);
        return reference;
    }

    // TODO: Unload

    /**
     * Get a reference to an existing resource of type T, but does not attempt to create it if not loaded.
     * If the resource does not exist, returns a null reference.
     *
     * @tparam T The underlying requested resource type
     * @param resource_id The resource id
     * @return A reference to the resource, if exists
     */
    template <ResourceConcept T>
    ResourceReference<T> get(StringId resource_id)
    {
        auto res = database.find(resource_id);
        if (res.has_value())
            return ResourceReference<T>(resource_id, res->handle, *this, reference_manager);

        return ResourceReference<T>(resource_id, INVALID_RESOURCE_HANDLE, *this, reference_manager);
    }

    /**
     * Allocated a resource of type T in the registry, returns a pointer to it (not a `ResourceReference`)
     * Resources allocated this way are considered loaded and can be used straight away
     *
     * @tparam T The type of the resource
     * @param resource_id The id of the resource
     * @param args The constructor arguments for the resource
     * @return A pointer to the resource
     */
    template <ResourceConcept T, typename... Args>
    Reference<T> allocate(const StringId resource_id, Args&&... args)
    {
        return allocate<T>(to_resource_handle(resource_id), std::forward<Args>(args)...);
    }

    /**
     * Allocated a resource of type T in the registry, returns a pointer to it (not a `ResourceReference`)
     * Resources allocated this way are considered loaded and can be used straight away
     *
     * @tparam T The type of the resource
     * @param handle A handle to the resource
     * @param args The constructor arguments for the resource
     * @return A pointer to the resource
     */
    template <ResourceConcept T, typename... Args>
    Reference<T> allocate(const ResourceHandle handle, Args&&... args)
    {
        // TODO: add some dependency checks?
        auto ref = make_reference<T>(std::forward<Args>(args)...);
        resources[handle] = ref;
        return ref;
    }

    /**
     * Directly loads a resource from a source (through the corresponding loader)
     *
     * @param meta The source metadata
     * @param source A type erased source
     * @return A pointer to the created resource
     */
    Reference<Resource> load_direct(const SourceMetadata& meta, const resources::ResourceSource& source);

protected:
    /**
     * Gets a pointer to the resource from a handle, if the resource is invalid, returns the invalid state instead
     *
     * @param handle The handle to the resource
     * @return A resource pointer if valid, the invalid state as a `ResourceState` enum otherwise
     */
    [[nodiscard]] std::expected<Reference<Resource>, ResourceState> get_resource(ResourceHandle handle) const;

    /**
     * Creates a new resource asynchronously in the registry and returns a handle to it.
     * If the resource exists already (either in pending or loaded), returns the existing handle
     *
     * @param resource_id The resource id.
     * @param type The type of the resource.
     * @return A handle to the resource
     */
    ResourceHandle create_resource(const StringId& resource_id, ResourceType type);

    /**
     * Much like the `create_resource` this function creates a resource and returns a hanlde, but blocks until the resource creation is done.
     * If the resource exists already (either in pending or loaded), returns the existing handle
     *
     * @param resource_id The resource id.
     * @param type The type of the resource.
     * @return A handle to the resource
     */
    ResourceHandle create_resource_immediate(const StringId& resource_id, ResourceType type);

    /**
     * Converts a string id to a resource handle.
     */
    static ResourceHandle to_resource_handle(const StringId& resource_id);

    Job<Reference<Resource>> load_resource(ResourceHandle handle);

private:
    template <ResourceConcept T>
    friend class ResourceReference;

    // Resource container, all resource are managed
    // TODO: use custom allocator to have the resources next to each other on the heap
    llvm::DenseMap<ResourceHandle, Reference<Resource>> resources;
    llvm::DenseSet<ResourceHandle> pending_resources;
    llvm::DenseSet<ResourceHandle> errored_resources;

    ResourceDatabase& database;
    ReferenceManager& reference_manager;
    jobs::Scheduler& scheduler;
    RendererContext& context;

    resources::LoaderFactory loader_factory;
};
} // portal
