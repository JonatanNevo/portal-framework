//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "resource.h"

#include "portal/engine/resources/new/reference_manager.h"
#include "portal/engine/resources/new/resource_registry.h"

namespace portal::ng
{

template <ResourceConcept T>
class ResourceReference
{
public:
    ~ResourceReference()
    {
        if (handle != INVALID_RESOURCE_HANDLE)
            reference_manager.unregister_reference(this);
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

        reference_manager.register_reference(this);
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

        reference_manager.move_reference(other, this);
    }

    ResourceReference& operator=(const ResourceReference& other)
    {
        if (other == this)
            return *this;

        reference_manager.unregister_reference(this);

        PORTAL_ASSERT(&reference_manager == &other.reference_manager, "Reference managers are not the same");
        PORTAL_ASSERT(&registry == &other.registry, "Resource registries are not the same");

        resource_id = other.resource_id;
        handle = other.handle;
        state = other.state;
        resource = other.resource;
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");

        reference_manager.register_reference(this);
        return *this;
    }

    ResourceReference& operator=(ResourceReference&& other) noexcept
    {
        if (other == this)
            return *this;

        reference_manager.unregister_reference(this);

        resource_id = std::exchange(other.resource_id, INVALID_STRING_ID);
        reference_manager = std::exchange(other.reference_manager, nullptr);
        handle = std::exchange(other.handle, INVALID_RESOURCE_HANDLE);
        state = std::exchange(other.state, ResourceState::Unknown);
        resource = std::exchange(other.resource, nullptr);

        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");

        reference_manager.move_reference(handle, this);
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
        // Check the state only when not yet loaded
        if (state != ResourceState::Loaded)
        {
            auto result = registry.get_resource(handle);
            if (result.has_value())
            {
                resource = dynamic_cast<T*>(result.value());
                if (!resource)
                    LOG_ERROR_TAG("Resource", "Failed to cast resource \"{}\" to type \"{}\"", resource_id, T::static_type_name());
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
            return resource;

        LOG_WARN_TAG("Resource", "Failed to fetch \"{}\" its state is \"{}\"", resource_id, state);
        return nullptr;
    }

    bool operator==(const ResourceReference& other) const
    {
        return handle == other.handle;
    }

private:
    friend class ResourceRegistry;
    friend class ReferenceManager;

    ResourceReference(
        const StringId& resource_id,
        const ResourceHandle handle,
        ResourceRegistry& registry,
        ReferenceManager& reference_manager
        ) :
        reference_manager(reference_manager),
        registry(registry),
        resource_id(resource_id),
        handle(handle)
    {
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");
    }

private:
    ReferenceManager& reference_manager;
    ResourceRegistry& registry;

    StringId resource_id;
    ResourceHandle handle;

    ResourceState state = ResourceState::Unknown;
    T* resource = nullptr;
};
}
