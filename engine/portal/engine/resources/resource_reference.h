//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "resources/resource.h"

#include "reference_manager.h"
#include "resource_registry.h"

namespace portal
{

template <ResourceConcept T>
class ResourceReference
{
public:
    ResourceReference() = default;
    explicit ResourceReference(nullptr_t) : ResourceReference() {};

    ~ResourceReference()
    {
        if (handle != INVALID_RESOURCE_HANDLE && reference_manager.has_value())
            reference_manager.value().get().unregister_reference(handle,this);
    }

    ResourceReference(const ResourceReference& other) :
        reference_manager(other.reference_manager),
        registry(other.registry),
        resource_id(other.resource_id),
        handle(other.handle),
        state(other.state),
        resource(other.resource)
    {
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");
        PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
        PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");

        reference_manager.value().get().register_reference(handle, this);
    }

    ResourceReference(ResourceReference&& other) noexcept :
        reference_manager(other.reference_manager),
        registry(other.registry),
        resource_id(std::exchange(other.resource_id, INVALID_STRING_ID)),
        handle(std::exchange(other.handle, INVALID_RESOURCE_HANDLE)),
        state(std::exchange(other.state, ResourceState::Unknown)),
        resource(std::exchange(resource, nullptr))
    {
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");
        PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
        PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");

        reference_manager.value().get().move_reference(handle, &other, this);
    }

    ResourceReference& operator=(const ResourceReference& other)
    {
        if (&other == this)
            return *this;


        if (reference_manager.has_value())
            reference_manager.value().get().unregister_reference(handle,this);

        resource_id = other.resource_id;
        handle = other.handle;
        state = other.state;
        resource = other.resource;
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");
        PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
        PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");

        if (handle != INVALID_RESOURCE_HANDLE && reference_manager.has_value())
            reference_manager.value().get().register_reference(handle,this);
        return *this;
    }

    ResourceReference& operator=(ResourceReference&& other) noexcept
    {
        if (&other == this)
            return *this;

        if (reference_manager.has_value())
            reference_manager.value().get().unregister_reference(handle, this);

        resource_id = std::exchange(other.resource_id, INVALID_STRING_ID);
        handle = std::exchange(other.handle, INVALID_RESOURCE_HANDLE);
        state = std::exchange(other.state, ResourceState::Unknown);
        resource = std::exchange(other.resource, nullptr);

        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");
        PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
        PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");

        if (handle != INVALID_RESOURCE_HANDLE && reference_manager.has_value())
            reference_manager.value().get().move_reference(handle, &other, this);
        return *this;
    }

    /**
     * Returns the current state of the resource in the registry,
     * @note in all other states than `ResourceState::Loaded` the underlying resource pointer will be null.
     *
     * @return The state of the resource in the registry
     */
    ResourceState get_state()
    {
        if (state == ResourceState::Null)
            return state;

        // Check the state only when not yet loaded
        if (state != ResourceState::Loaded)
        {
            auto result = registry->get().get_resource(handle);
            if (result.has_value())
            {
                resource = reference_cast<T, Resource>(result.value());
                if (!resource)
                    LOG_ERROR_TAG("Resource", "Failed to cast resource \"{}\" to type \"{}\"", resource_id, T::static_type());
            }

            state = result.error_or(ResourceState::Loaded);
        }
        return state;
    }

    /**
     * Checks if the resource in valid in the registry (loaded)
     * @note this function will lazily load the resource into the reference if the resource was unloaded before and now is loaded
     *
     * @return True if the resource is loaded, false otherwise
     */
    bool is_valid()
    {
        return get_state() == ResourceState::Loaded;
    }

    /**
     * Returns the underlying pointer to the resource, with nullptr otherwise.
     * @note Please use after validation the state of the reference using `is_valid` or `get_state`
     *
     * @return A pointer to the underlying resource, nullptr otherwise
     */
    T* get()
    {
        if (is_valid())
            return resource.get();

        LOG_WARN_TAG("Resource", "Failed to fetch \"{}\" its state is \"{}\"", resource_id, state);
        return nullptr;
    }

    T* operator->() { return get(); }
    T& operator*() { return *get(); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    /**
     * @return The underlying `Reference` class to the resource
     */
    Reference<T> underlying()
    {
        return resource;
    }

    Reference<T> underlying() const
    {
        return resource;
    }

    bool operator==(const ResourceReference& other) const
    {
        return handle == other.handle;
    }

    template <ResourceConcept U>
    ResourceReference<U> cast()
    {
        auto* casted = reference_cast<U, T>(resource);
        if (!casted)
        {
            LOG_ERROR_TAG("Resource", "Failed to cast resource \"{}\" to type \"{}\"", resource_id, U::static_type());
            return ResourceReference<U>(resource_id, INVALID_RESOURCE_HANDLE, registry, reference_manager);
        }

        return ResourceReference<U>(resource_id, handle, registry, reference_manager);
    }

private:
    friend class ResourceRegistry;
    friend class ReferenceManager;

    ResourceReference(
        const StringId& resource_id,
        const ResourceHandle handle,
        ResourceRegistry& registry,
        ReferenceManager& reference_manager
        ) : reference_manager(reference_manager),
            registry(registry),
            resource_id(resource_id),
            handle(handle),
            state(ResourceState::Unknown)
    {
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");
        reference_manager.register_reference(handle, this);
        get_state();
    }

private:
    std::optional<std::reference_wrapper<ReferenceManager>> reference_manager = std::nullopt;
    std::optional<std::reference_wrapper<ResourceRegistry>> registry = std::nullopt;

    StringId resource_id = INVALID_STRING_ID;
    ResourceHandle handle = INVALID_RESOURCE_HANDLE;

    ResourceState state = ResourceState::Null;
    Reference<T> resource = nullptr;
};
}
