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
    explicit ResourceReference(std::nullptr_t) : ResourceReference() {};

    ~ResourceReference()
    {
        if (resource_id != INVALID_STRING_ID && reference_manager.has_value())
            reference_manager.value().get().unregister_reference(resource_id, this);
    }

    ResourceReference(const ResourceReference& other) :
        reference_manager(other.reference_manager),
        registry(other.registry),
        resource_id(other.resource_id),
        state(other.state),
        resource(other.resource)
    {
        PORTAL_ASSERT(resource_id != INVALID_STRING_ID, "Resource handle is invalid");
        PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
        PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");
        PORTAL_ASSERT(state != ResourceState::Loaded || resource != nullptr, "Resource is empty");

        reference_manager.value().get().register_reference(resource_id, this);
    }

    ResourceReference(ResourceReference&& other) noexcept :
        reference_manager(other.reference_manager),
        registry(other.registry),
        resource_id(std::exchange(other.resource_id, INVALID_STRING_ID)),
        state(std::exchange(other.state, ResourceState::Unknown)),
        resource(std::exchange(other.resource, nullptr))
    {
        PORTAL_ASSERT(resource_id != INVALID_STRING_ID, "Resource handle is invalid");
        PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
        PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");
        PORTAL_ASSERT(state != ResourceState::Loaded || resource != nullptr, "Resource is empty");

        reference_manager.value().get().move_reference(resource_id, &other, this);
    }

    ResourceReference& operator=(const ResourceReference& other)
    {
        if (&other == this)
            return *this;


        if (reference_manager.has_value())
            reference_manager.value().get().unregister_reference(resource_id, this);

        resource_id = other.resource_id;
        state = other.state;
        resource = other.resource;
        reference_manager = other.reference_manager;
        registry = other.registry;

        PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
        PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");
        PORTAL_ASSERT(state != ResourceState::Loaded || resource != nullptr, "Resource is empty");

        if (resource_id != INVALID_STRING_ID && reference_manager.has_value())
            reference_manager.value().get().register_reference(resource_id, this);
        return *this;
    }

    ResourceReference& operator=(ResourceReference&& other) noexcept
    {
        if (&other == this)
            return *this;

        if (reference_manager.has_value())
            reference_manager.value().get().unregister_reference(resource_id, this);

        resource_id = std::exchange(other.resource_id, INVALID_STRING_ID);
        state = std::exchange(other.state, ResourceState::Unknown);
        resource = std::exchange(other.resource, nullptr);
        reference_manager = std::exchange(other.reference_manager, std::nullopt);
        registry = std::exchange(other.registry, std::nullopt);

        PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
        PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");
        PORTAL_ASSERT(state != ResourceState::Loaded || resource != nullptr, "Resource is empty");

        if (resource_id != INVALID_STRING_ID && reference_manager.has_value())
            reference_manager.value().get().move_reference(resource_id, &other, this);
        return *this;
    }

    /**
     * Returns the current state of the resource in the registry,
     * @note in all other states than `ResourceState::Loaded` the underlying resource pointer will be null.
     *
     * @return The state of the resource in the registry
     */
    ResourceState get_state() const
    {
        if (state == ResourceState::Null)
            return state;

        // Check the state only when not yet loaded
        if (state != ResourceState::Loaded)
        {
            const auto result = registry->get().get_resource(resource_id);
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
    bool is_valid() const
    {
        return get_state() == ResourceState::Loaded;
    }

    /**
     * Returns the underlying pointer to the resource, with nullptr otherwise.
     * @note Please use after validation the state of the reference using `is_valid` or `get_state`
     *
     * @return A pointer to the underlying resource, nullptr otherwise
     */
    T* get() const
    {
        if (is_valid())
            return resource.get();

        LOG_WARN_TAG("Resource", "Failed to fetch \"{}\" its state is \"{}\"", resource_id, state);
        return nullptr;
    }

    T* operator->()
    {
        auto* ptr = get();
        PORTAL_ASSERT(ptr, "Resource is null");
        return ptr;
    }

    T& operator*()
    {
        auto* ptr = get();
        PORTAL_ASSERT(ptr, "Resource is null");
        return *ptr;
    }

    const T* operator->() const
    {
        auto* ptr = get();
        PORTAL_ASSERT(ptr, "Resource is null");
        return ptr;
    }

    const T& operator*() const
    {
        auto* ptr = get();
        PORTAL_ASSERT(ptr, "Resource is null");
        return *ptr;
    }

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
        return resource_id == other.resource_id;
    }

    template <ResourceConcept U> requires std::is_base_of_v<T, U>
    ResourceReference<U> cast() const
    {
        auto casted = reference_cast<U, T>(resource);
        if (!casted)
        {
            LOG_ERROR_TAG("Resource", "Failed to cast resource \"{}\" to type \"{}\"", resource_id, U::static_type());
            return ResourceReference<U>(resource_id, registry.value().get(), reference_manager.value().get());
        }

        return ResourceReference<U>(resource_id, registry.value().get(), reference_manager.value().get());
    }

private:
    friend class ResourceRegistry;
    friend class ReferenceManager;

    template <ResourceConcept U>
    friend class ResourceReference;

    ResourceReference(
        const StringId& resource_id,
        ResourceRegistry& registry,
        ReferenceManager& reference_manager
        ) : reference_manager(reference_manager),
            registry(registry),
            resource_id(resource_id),
            state(ResourceState::Unknown)
    {
        if (resource_id != INVALID_STRING_ID)
        {
            reference_manager.register_reference(resource_id, this);
            get_state();
        }
        else
        {
            state = ResourceState::Null;
        }
    }

private:
    std::optional<std::reference_wrapper<ReferenceManager>> reference_manager = std::nullopt;
    std::optional<std::reference_wrapper<ResourceRegistry>> registry = std::nullopt;

    StringId resource_id = INVALID_STRING_ID;

    mutable ResourceState state = ResourceState::Null;
    mutable Reference<T> resource = nullptr;
};
}
