//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once


#include "resource.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <portal/core/jobs/scheduler.h>

#include "portal/engine/resources/utils.h"
#include "portal/engine/resources/new/reference_manager.h"
#include "portal/engine/resources/new/database/resource_database.h"
#include "portal/engine/resources/new/loader/loader_factory.h"

namespace portal::renderer::vulkan {
class VulkanTexture;
}

namespace portal::ng
{

template <ResourceConcept T>
class ResourceReference;

class ResourceRegistry
{
public:
    ResourceRegistry(ReferenceManager& ref_manager, ResourceDatabase& database, jobs::Scheduler& scheduler, renderer::vulkan::VulkanContext& context);

    ~ResourceRegistry() noexcept;

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

        auto reference = ResourceReference<T>(resource_id, handle, this);
        return reference;
    }

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
        auto type = utils::to_resource_type<T>();
        auto res = database.find(resource_id);

        if (res)
            return ResourceReference<T>(resource_id, res->handle, this);

        return ResourceReference<T>(resource_id, INVALID_RESOURCE_HANDLE, this);
    }

protected:
    /**
     * Gets a pointer to the resource from a handle, if the resource is invalid, returns the invalid state instead
     *
     * @param handle The handle to the resource
     * @return A resource pointer if valid, the invalid state as a `ResourceState` enum otherwise
     */
    [[nodiscard]] std::expected<Resource*, ResourceState> get_resource(ResourceHandle handle) const;

    /**
     * Creates a new resource in the registry and returns a handle to it.
     * If the resource exists already (either in pending or loaded), returns the existing handle
     *
     * @param resource_id The resource id.
     * @param type The type of the resource.
     * @return A handle to the resource
     */
    ResourceHandle create_resource(const StringId& resource_id, ResourceType type);

    /**
     * Converts a string id to a resource handle.
     */
    static ResourceHandle to_resource_handle(const StringId& resource_id);

    Job<Resource*> load_resource(ResourceHandle handle);

private:
    template <ResourceConcept T>
    friend class ResourceReference;

    friend class resources::TextureLoader;

    // Resource container, all resource are managed
    // TODO: use custom allocator to have the resources next to each other on the heap
    llvm::DenseMap<ResourceHandle, Resource*> resources;
    llvm::DenseSet<ResourceHandle> pending_resources;
    llvm::DenseSet<ResourceHandle> errored_resources;

    ResourceDatabase& database;
    ReferenceManager& reference_manager;
    jobs::Scheduler& scheduler;
    renderer::vulkan::VulkanContext& context;

    resources::LoaderFactory loader_factory;
};
} // portal