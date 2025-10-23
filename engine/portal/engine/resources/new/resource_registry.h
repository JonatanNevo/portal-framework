//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <future>
#include <coroutine>

#include <concurrentqueue/concurrentqueue.h>

#include "resource.h"
#include "llvm/ADT/DenseMap.h"
#include "portal/platform/core/hal/thread.h"
#include "portal/core/concurrency/reentrant_spin_lock.h"
#include "portal/engine/resources/utils.h"
#include "portal/engine/resources/new/reference_manager.h"

namespace portal::ng
{

template <ResourceConcept T>
class ResourceReference;

class ResourceRegistry
{
public:
    ResourceRegistry(ReferenceManager& ref_manager);

    /**
     * Request load to a resource based on its unique id and returns a reference.
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
        auto type = resources::utils::to_resource_type<T>();
        auto handle = create_resource(resource_id, type);

        auto reference = ResourceReference<T>(resource_id, handle, this);
        reference_manager.register_reference(&reference);
        return reference;
    }

    /**
     * Gets a pointer to the resource from a handle, if the resource is invalid, returns the invalid state instead
     *
     * @param handle The handle to the resource
     * @return A resource pointer if valid, the invalid state as a `ResourceState` enum otherwise
     */
    [[nodiscard]] std::expected<Resource*, ResourceState> get_resource(ResourceHandle handle) const;

private:
    /**
     * Creates a new resource in the registry and returns a handle to it.
     * If the resource exists already (either in pending or loaded), returns the existing handle
     *
     * @param resource_id The resource id.
     * @param type The type of the resource.
     * @return A handle to the resource
     */
    ResourceHandle create_resource(const StringId& resource_id, ResourceType type);

    static ResourceHandle to_resource_handle(const StringId& resource_id);

    void resource_load_loop(const std::stop_token& stoken);

    void request_resource(const StringId& resource_id, ResourceType type);

private:
    class ResourceRequest
    {
        [[maybe_unused]] ResourceHandle handle;
        std::promise<Resource> resource_promise;
    };

    llvm::DenseMap<ResourceHandle, std::future<Resource>> pending_resources;
    llvm::DenseMap<ResourceHandle, Resource> resources;

    ReferenceManager& reference_manager;

    ReentrantSpinLock<> resource_lock;
    Thread resource_loader_thread;
    moodycamel::ConcurrentQueue<ResourceRequest> resource_load_queue;
};
} // portal