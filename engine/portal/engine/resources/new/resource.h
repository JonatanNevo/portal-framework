//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <expected>
#include <future>

#include "portal/engine/resources/resource_types.h"
#include "portal/engine/resources/utils.h"
#include "portal/engine/strings/string_id.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

namespace portal::ng
{

using ResourceHandle = size_t;
constexpr ResourceHandle INVALID_RESOURCE_HANDLE = 0;

class Resource
{
public:
    explicit Resource(const StringId& id) : id(id) {}
    virtual ~Resource() = default;

    static StringId static_type_name() { return STRING_ID("Resource"); }

    [[nodiscard]] const StringId& get_id() const { return id; }

private:
    StringId id;
};

template <typename T>
concept ResourceConcept = requires {
    { T::static_type_name() } -> std::same_as<StringId>;
    std::is_base_of_v<Resource, T>;
};

template<ResourceConcept T>
class ResourceReference;

class ResourceRegistry
{
public:

    /**
     * Gets a reference to a certain resource based on its unique id.
     * The returned reference is invalid until the resource is loaded, once its loaded it can be accessed through the `ResourceReference` api
     *
     * @note Resources cannot have an `invalid state` but a reference can have one, make sure to test it before using the underlying resource.
     * @note the resource id is different from the resource handle, while both are unique per resource.
     *
     * @tparam T The underlying requested resource type
     * @param resource_id The resource id
     * @return A reference to the resource
     */
    template<ResourceConcept T>
    ResourceReference<T> get(StringId resource_id)
    {
        auto type = resources::utils::to_resource_type<T>();
        auto handle = create_resource(resource_id, type);

        auto reference = ResourceReference<T>(resource_id, handle, this);
        register_reference(&reference);
        return reference;
    }

private:
    template<ResourceConcept T>
    friend class ResourceReference;

    /**
     * Creates a new resource in the registry and returns a handle to it.
     * If the resource exists already (either in pending or loaded), returns the existing handle
     *
     * @param resource_id The resource id.
     * @param type The type of the resource.
     * @return A handle to the resource
     */
    ResourceHandle create_resource(StringId resource_id, ResourceType type);

    /**
     * Gets a pointer to the resource from a handle, if the resource is invalid, returns the invalid state instead
     *
     * @param handle The handle to the resource
     * @return A resource pointer if valid, the invalid state as a `ResourceState` enum otherwise
     */
    std::expected<Resource*, ResourceState> get_resource(ResourceHandle handle) const;

    /**
     * Registers a new reference for the reference counting
     * @note we are using void* here because `ResourceReference` is a template function, and we don't need to access it directly from the registry
     *
     * @param reference a void* pointer to the reference.
     */
    void register_reference(void* reference);

    /**
     * Removes a reference from the reference counting
     *
     * @param reference a void* pointer to the reference.
     */
    void unregister_reference(void* reference);

    /**
     * Switches between two references in the reference counting, the same as calling `unregister(old) register(new)` but makes sure that there is always
     * a valid reference (used in the ResourceReference move operators)
     *
     * @param old_ref The old reference (as a void*)
     * @param new_ref The new reference (as a void*)
     */
    void move_reference(void* old_ref, void* new_ref);

private:
    llvm::DenseMap<ResourceHandle, ResourceType> resources;
    llvm::DenseMap<ResourceHandle, Resource> loaded_resources;

    llvm::DenseMap<ResourceHandle, llvm::SmallVector<void*>> references;
};

template <ResourceConcept T>
class ResourceReference
{
public:
    ~ResourceReference()
    {
        if (handle != INVALID_RESOURCE_HANDLE)
            registry->unregister_reference(this);
    }

    ResourceReference(const ResourceReference& other) :
        resource_id(other.resource_id),
        registry(other.registry),
        handle(other.handle),
        state(other.state),
        resource(other.resource)
    {
        PORTAL_ASSERT(registry, "Resource registry is null");
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");

        registry->register_reference(this);
    }

    ResourceReference(ResourceReference&& other) noexcept :
        resource_id(std::exchange(other.resource_id, INVALID_STRING_ID)),
        registry(std::exchange(other.registry, nullptr)),
        handle(std::exchange(other.handle, INVALID_RESOURCE_HANDLE)),
        state(std::exchange(other.state, ResourceState::Unknown)),
        resource(std::exchange(resource, nullptr))
    {
        PORTAL_ASSERT(registry, "Resource registry is null");
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");

        registry->move_reference(other, this);
    }

    ResourceReference& operator=(const ResourceReference& other)
    {
        if (other == this)
            return *this;

        registry->unregister_reference(this);

        resource_id = other.resource_id;
        registry = other.registry;
        handle = other.handle;
        state = other.state;
        resource = other.resource;
        PORTAL_ASSERT(registry, "Resource registry is null");
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");

        registry->register_reference(this);
        return *this;
    }

    ResourceReference& operator=(ResourceReference&& other) noexcept
    {
        if (other == this)
            return *this;

        registry->unregister_reference(this);

        resource_id = std::exchange(other.resource_id, INVALID_STRING_ID);
        registry = std::exchange(other.registry, nullptr);
        handle = std::exchange(other.handle, INVALID_RESOURCE_HANDLE);
        state = std::exchange(other.state, ResourceState::Unknown);
        resource = std::exchange(other.resource, nullptr);

        PORTAL_ASSERT(registry, "Resource registry is null");
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");

        registry->move_reference(handle, this);
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
            auto result = registry->get_resource(handle);
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

    ResourceReference(const StringId& resource_id, const ResourceHandle handle, ResourceRegistry* registry) : resource_id(resource_id),
        registry(registry),
        handle(handle)
    {
        PORTAL_ASSERT(registry, "Resource registry is null");
        PORTAL_ASSERT(handle != INVALID_RESOURCE_HANDLE, "Resource handle is invalid");
    }

private:
    StringId resource_id;
    ResourceRegistry* registry;
    ResourceHandle handle;

    ResourceState state = ResourceState::Unknown;
    T* resource = nullptr;
};

} // portal
