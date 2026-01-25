//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file resource_reference.h
 * @brief Smart handle for asynchronously-loaded resources
 *
 * This file defines ResourceReference<T>, the primary user-facing API for working with
 * resources in the Portal Framework. Unlike Reference<T> (which is std::shared_ptr<T>),
 * ResourceReference provides asynchronous loading semantics with state tracking.
 *
 * @see ResourceRegistry for loading resources
 * @see ResourceState for the state machine
 * @see Reference for the underlying shared pointer type
 */

#pragma once

#include "resources/resource.h"
#include <portal/serialization/serialize.h>
#include <portal/serialization/archive.h>

#include "reference_manager.h"
#include "resource_registry.h"

namespace portal
{
/**
 * @class ResourceReference
 * @brief Type-safe smart handle for asynchronously-loaded resources
 *
 * ResourceReference<T> is the primary interface for accessing resources loaded by the
 * ResourceRegistry. It provides a handle-based API where references can exist before
 * the resource finishes loading, enabling non-blocking async loading patterns.
 *
 * Key Concepts:
 *
 * **Handle vs Resource**:
 * A ResourceReference is a handle to a resource, not the resource itself. The actual
 * resource lives in the ResourceRegistry's internal storage. Multiple references can
 * point to the same resource, and references are cheap to copy.
 *
 * **Lazy State Synchronization**:
 * References cache their state (Unknown/Pending/Loaded/Error) and only query the
 * registry when needed. Calling get_state() or is_valid() triggers a registry lookup
 * if the cached state isn't Loaded. This avoids redundant lookups and enables efficient
 * state polling.
 *
 * **Reference Counting**:
 * The ReferenceManager tracks how many ResourceReferences point to each resource. This
 * enables future features like automatic unloading when reference counts drop to zero.
 * Copy/move operations automatically update the reference counts.
 *
 * **Thread Safety**:
 * State queries are thread-safe. The registry uses internal locking to ensure state
 * transitions are atomic. However, the underlying resource (accessed via get()) must
 * be used in accordance with its own thread-safety guarantees.
 *
 * Distinction from Reference<T>:
 *
 * - `Reference<T>` = `std::shared_ptr<T>` - Generic ownership for any engine object
 * - `ResourceReference<T>` = Smart handle for user assets with async loading semantics
 *
 * Once a resource is loaded in the registry, it's always valid. But a
 * ResourceReference might point to a resource that's still loading, failed, or doesn't exist.
 *
 * Usage Example (Async Loading):
 * @code
 * // Request async load (returns immediately)
 * auto texture_ref = registry.load<TextureResource>(STRING_ID("textures/albedo.png"));
 *
 * // First frame: resource is probably still loading
 * if (texture_ref.is_valid()) {
 *     // This branch won't execute yet
 *     auto& texture = texture_ref.get();
 *     renderer.bind_texture(texture);
 * } else {
 *     // Show loading placeholder
 *     renderer.bind_texture(default_texture);
 * }
 *
 * // Later frames: resource finishes loading
 * if (texture_ref.is_valid()) {
 *     // Now this executes - texture is ready
 *     auto& texture = texture_ref.get();
 *     renderer.bind_texture(texture);
 * }
 * @endcode
 *
 * Usage Example (Synchronous Loading):
 * @code
 * // Block until loaded (for critical startup resources)
 * auto mesh_ref = registry.immediate_load<MeshResource>(STRING_ID("models/character.gltf"));
 * // mesh_ref.is_valid() is guaranteed true here (or Error if load failed)
 * @endcode
 *
 * Usage Example (Error Handling):
 * @code
 * auto shader_ref = registry.load<ShaderResource>(STRING_ID("shaders/pbr.slang"));
 *
 * switch (shader_ref.get_state()) {
 *     case ResourceState::Loaded:
 *         // Ready to use
 *         break;
 *     case ResourceState::Pending:
 *         // Still loading, try again next frame
 *         break;
 *     case ResourceState::Error:
 *         // Load failed, use fallback
 *         LOG_ERROR("Failed to load shader");
 *         shader_ref = registry.get<ShaderResource>(STRING_ID("shaders/fallback.slang"));
 *         break;
 *     case ResourceState::Missing:
 *         // File doesn't exist
 *         LOG_ERROR("Shader file not found");
 *         break;
 * }
 * @endcode
 *
 * @tparam T The resource type (must satisfy ResourceConcept)
 *
 * @note Default-constructed references are in Null state and is_valid() returns false
 * @note Moved-from references become Null
 * @note operator-> and operator* dont check if the resource isn't loaded - use is_valid() first
 *
 * @see ResourceRegistry::load() for async loading
 * @see ResourceRegistry::immediate_load() for blocking loading
 * @see ResourceRegistry::get() for retrieving already-loaded resources
 * @see ResourceState for all possible states
 * @see ReferenceManager for reference counting details
 */
template <ResourceConcept T>
class ResourceReference
{
public:
    /** @brief Default constructor - creates a Null reference */
    ResourceReference() = default;

    /** @brief Nullptr constructor - creates a Null reference */
    explicit ResourceReference(std::nullptr_t) : ResourceReference() {};

    ResourceReference(const StringId& resource_id)
        : resource_id(resource_id), state(ResourceState::Missing) {}

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
        if (state != ResourceState::Null && state != ResourceState::Missing)
        {
            PORTAL_ASSERT(resource_id != INVALID_STRING_ID, "Resource handle is invalid");
            PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
            PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");
            PORTAL_ASSERT(state != ResourceState::Loaded || resource != nullptr, "Resource is empty");

            reference_manager.value().get().move_reference(resource_id, &other, this);
        }
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

        if (state != ResourceState::Null && state != ResourceState::Missing)
        {
            PORTAL_ASSERT(reference_manager.has_value(), "Invalid reference manager");
            PORTAL_ASSERT(registry.has_value(), "Invalid resource registry");
            PORTAL_ASSERT(state != ResourceState::Loaded || resource != nullptr, "Resource is empty");

            if (resource_id != INVALID_STRING_ID && reference_manager.has_value())
                reference_manager.value().get().move_reference(resource_id, &other, this);
        }

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

    ResourceDirtyFlags get_dirty() const
    {
        if (state != ResourceState::Loaded)
            return ResourceDirtyBits::Clean;

        return registry->get().get_dirty(resource_id);
    }

    void set_dirty(const ResourceDirtyFlags dirty) const
    {
        if (state != ResourceState::Loaded)
            return;

        return registry->get().set_dirty(resource_id, dirty);
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

    [[nodiscard]] StringId get_resource_id() const { return resource_id; }

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


template <typename T>
struct fmt::formatter<portal::ResourceReference<T>>
{
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const portal::ResourceReference<T>& ref, FormatContext& ctx) const
    {
        return fmt::format_to(
            ctx.out(),
            "ResourceReference<{}>(id={}, state={})",
            glz::type_name<T>,
            ref.get_resource_id(),
            ref.get_state()
        );
    }
};
