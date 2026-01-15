//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "llvm/ADT/SmallVector.h"
#include "portal/core/debug/assert.h"
#include "portal/core/strings/string_id.h"

#include "entity_iterators.h"

namespace portal
{
/**
 * @brief Lightweight wrapper around EnTT's handle providing Portal's component access API.
 *
 * The Entity class wraps entt::handle (which combines an entity ID with a registry reference)
 * and provides Portal-specific component manipulation and parent-child hierarchy support.
 * Entity objects are value types that can be freely copied and passed around - they don't
 * own the underlying entity data, just reference it.
 *
 * Key features:
 * - Component management (add, remove, get, patch)
 * - Parent-child hierarchy relationships via RelationshipComponent
 * - Iteration over children and descendants
 * - Type-safe component access with compile-time checks
 *
 * All entities created through Registry automatically receive default components:
 * - RelationshipComponent
 * - TransformComponent
 *
 * @par Example Usage:
 * @code
 * // Entity is obtained from Registry, not constructed directly
 * auto player = registry.create_entity(STRING_ID("player"));
 *
 * // Add components
 * player.add_component<HealthComponent>(100.0f);
 * player.add_component<VelocityComponent>(vec3{0, 0, 0});
 *
 * // Access components
 * auto& health = player.get_component<HealthComponent>();
 * health.value -= 10.0f;
 *
 * // Optional access
 * if (auto* armor = player.try_get_component<ArmorComponent>()) {
 *     armor->durability -= 5;
 * }
 *
 * // Check component existence
 * if (player.has_component<TransformComponent>()) {
 *     // All entities have Transform by default
 * }
 *
 * // Parent-child hierarchy
 * auto weapon = registry.create_entity(STRING_ID("sword"));
 * weapon.set_parent(player);
 *
 * // Iterate children
 * for (auto child : player.children()) {
 *     // Process direct children
 * }
 * @endcode
 *
 * @note Entity is a handle - it doesn't own the entity data and can become invalid if
 *       the entity is destroyed. Use is_valid() to check validity.
 * @note Component access methods assert in debug builds if the component doesn't exist.
 *
 * @see Registry for entity creation and destruction
 * @see RelationshipComponent for hierarchy implementation details
 */
class Entity
{
public:
    /**
     * @brief Constructs an invalid (null) entity.
     *
     * Default-constructed entities are invalid and cannot be used until assigned
     * a valid entity handle.
     */
    Entity() = default;

    /**
     * @brief Constructs an Entity wrapper from a raw entity ID and registry.
     *
     * @param entity The raw EnTT entity identifier
     * @param reg Reference to the registry containing the entity
     *
     * @note This is typically called by Registry methods, not user code.
     */
    Entity(entt::entity entity, entt::registry& reg);

    /**
     * @brief Constructs an Entity wrapper from an EnTT handle.
     *
     * @param handle The EnTT handle (entity + registry)
     *
     * @note This is typically called by Registry methods, not user code.
     */
    Entity(entt::handle handle);

    /**
     * @brief Adds a component to this entity with constructor arguments.
     *
     * Constructs the component in-place using the provided arguments and attaches
     * it to this entity. Asserts if the component already exists.
     *
     * @tparam T The component type to add
     * @tparam Args Argument types for the component constructor
     * @param args Arguments forwarded to the component constructor
     * @return Reference to the newly created component
     *
     * @par Example:
     * @code
     * auto& health = entity.add_component<HealthComponent>(100.0f);
     * entity.add_component<VelocityComponent>(vec3{1, 0, 0});
     * @endcode
     *
     * @note Asserts in debug builds if the component already exists.
     */
    template <typename T, typename... Args>
    T& add_component(Args&&... args)
    {
        PORTAL_ASSERT(!has_component<T>(), "Entity already has component of type T");
        return handle.emplace<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Adds an empty (tag) component to this entity.
     *
     * Specialized overload for empty component types (tags) that don't require
     * constructor arguments. Useful for marking entities with specific properties.
     *
     * @tparam T The empty component type to add (must satisfy std::is_empty_v)
     *
     * @par Example:
     * @code
     * entity.add_component<PlayerTag>();
     * entity.add_component<TransformDirtyTag>();
     * @endcode
     *
     * @note Asserts in debug builds if the component already exists.
     */
    template <typename T> requires std::is_empty_v<T>
    void add_component()
    {
        PORTAL_ASSERT(!has_component<T>(), "Entity already has component of type T");
        handle.emplace<T>();
    }

    /**
     * @brief Modifies a component through EnTT's patch mechanism.
     *
     * Applies the provided functors to the component, triggering EnTT's update
     * signal. This allows systems to register callbacks (via on_component_changed)
     * that react to component modifications.
     *
     * @tparam T The component type to patch
     * @tparam Func Functor types that accept T& or const T&
     * @param func One or more functors to apply to the component
     * @return Reference to the modified component
     *
     * @par Example:
     * @code
     * // Modify transform and trigger dirty flag
     * entity.patch_component<TransformComponent>([](auto& t) {
     *     t.position += vec3{1, 0, 0};
     * });
     * @endcode
     *
     * @note Asserts if the component doesn't exist.
     * @note Use this instead of direct modification when you want change detection.
     */
    template <typename T, typename... Func>
    T& patch_component(Func&&... func)
    {
        PORTAL_ASSERT(has_component<T>(), "Entity does not have component of type T");
        return handle.patch<T>(std::forward<Func>(func)...);
    }

    /**
     * @brief Removes a component from this entity.
     *
     * Destroys the component and removes it from the entity. Triggers EnTT's
     * destruction signal, allowing systems to clean up via on_component_removed.
     *
     * @tparam T The component type to remove
     *
     * @par Example:
     * @code
     * if (entity.has_component<TemporaryEffectComponent>()) {
     *     entity.remove_component<TemporaryEffectComponent>();
     * }
     * @endcode
     *
     * @note Asserts if the component doesn't exist.
     */
    template <typename T>
    void remove_component() const
    {
        PORTAL_ASSERT(has_component<T>(), "Entity does not have component of type T");
        [[maybe_unused]] auto deleted = handle.remove<T>();
        PORTAL_ASSERT(deleted == 1, "Failed to remove component of type T");
    }

    /**
     * @brief Sets the parent of this entity in the hierarchy.
     *
     * Establishes a parent-child relationship by updating RelationshipComponent.
     * If this entity already has a parent, it's first removed from that parent's
     * child list before being added to the new parent.
     *
     * @param parent The entity to become this entity's parent
     *
     * @par Example:
     * @code
     * auto player = registry.create_entity(STRING_ID("player"));
     * auto weapon = registry.create_entity(STRING_ID("sword"));
     * weapon.set_parent(player); // weapon is now child of player
     * @endcode
     *
     * @see create_child_entity for creating entities with parents
     * @see RelationshipComponent for implementation details
     */
    void set_parent(Entity parent);

    /**
     * @brief Removes a child from this entity's child list.
     *
     * Breaks the parent-child relationship, making the child a top-level entity.
     * The child entity itself is not destroyed, only orphaned.
     *
     * @param child The child entity to remove
     * @return true if the child was found and removed, false otherwise
     *
     * @par Example:
     * @code
     * if (player.remove_child(weapon)) {
     *     // weapon is no longer attached to player
     * }
     * @endcode
     */
    bool remove_child(Entity child);

    /**
     * @brief Retrieves a reference to a component.
     *
     * Returns a reference to the specified component type. The component must exist
     * or an assertion will fire in debug builds.
     *
     * @tparam T The component type to retrieve
     * @return Reference to the component
     *
     * @par Example:
     * @code
     * auto& transform = entity.get_component<TransformComponent>();
     * transform.position = vec3{10, 0, 0};
     * @endcode
     *
     * @note Asserts if the component doesn't exist. Use try_get_component() for optional access.
     */
    template <typename T>
    [[nodiscard]] T& get_component()
    {
        PORTAL_ASSERT(has_component<T>(), "Entity does not have component of type T");
        return handle.get<T>();
    }

    /**
     * @brief Retrieves a const reference to a component.
     *
     * @tparam T The component type to retrieve
     * @return Const reference to the component
     *
     * @note Asserts if the component doesn't exist.
     */
    template <typename T>
    [[nodiscard]] const T& get_component() const
    {
        PORTAL_ASSERT(has_component<T>(), "Entity does not have component of type T");
        return handle.get<T>();
    }

    /**
     * @brief Attempts to retrieve a pointer to a component.
     *
     * Returns a pointer to the component if it exists, or nullptr if not.
     * This is the safe way to access optional components without assertions.
     *
     * @tparam T The component type to retrieve
     * @return Pointer to the component, or nullptr if not found
     *
     * @par Example:
     * @code
     * if (auto* armor = entity.try_get_component<ArmorComponent>()) {
     *     armor->durability -= damage;
     * } else {
     *     // Entity has no armor, apply damage directly to health
     * }
     * @endcode
     */
    template <typename T>
    [[nodiscard]] T* try_get_component()
    {
        PORTAL_ASSERT(is_valid(), "Entity is invalid");
        return handle.try_get<T>();
    }

    /**
     * @brief Attempts to retrieve a const pointer to a component.
     *
     * @tparam T The component type to retrieve
     * @return Const pointer to the component, or nullptr if not found
     */
    template <typename T>
    [[nodiscard]] const T* try_get_component() const
    {
        PORTAL_ASSERT(is_valid(), "Entity is invalid");
        return handle.try_get<T>();
    }

    /**
     * @brief Checks if the entity has all specified components.
     *
     * Returns true only if the entity possesses all of the specified component types.
     * Supports checking multiple components at once.
     *
     * @tparam T Component types to check for
     * @return true if all components exist, false otherwise
     *
     * @par Example:
     * @code
     * if (entity.has_component<TransformComponent, RenderComponent>()) {
     *     // Entity is renderable
     * }
     * @endcode
     */
    template <typename... T>
    [[nodiscard]] bool has_component() const
    {
        PORTAL_ASSERT(is_valid(), "Entity is invalid");
        return handle.all_of<T...>();
    }

    /**
     * @brief Checks if the entity has any of the specified components.
     *
     * Returns true if the entity possesses at least one of the specified component types.
     *
     * @tparam T Component types to check for
     * @return true if any component exists, false otherwise
     *
     * @par Example:
     * @code
     * if (entity.has_any<HealthComponent, ShieldComponent>()) {
     *     // Entity has some form of protection
     * }
     * @endcode
     */
    template <typename... T>
    [[nodiscard]] bool has_any() const
    {
        PORTAL_ASSERT(is_valid(), "Entity is invalid");

        return handle.any_of<T...>();
    }

    /**
     * @brief Checks if this entity handle is valid.
     *
     * An entity is invalid if it's default-constructed or if the underlying
     * entity has been destroyed.
     *
     * @return true if the entity is valid, false otherwise
     */
    [[nodiscard]] bool is_valid() const;

    /**
     * @brief Returns the raw EnTT entity identifier.
     *
     * @return The underlying entt::entity ID
     */
    [[nodiscard]] entt::entity get_id() const;

    /**
     * @brief Returns the entity's name.
     *
     * @return The entity's StringId name, or "Unnamed" if no NameComponent exists
     */
    [[nodiscard]] StringId get_name() const;

    /**
     * @brief Converts the entity to its numeric ID.
     *
     * @return The entity ID as uint32_t
     */
    [[nodiscard]] operator uint32_t() const;

    /**
     * @brief Converts to the raw EnTT entity type.
     *
     * @return The underlying entt::entity
     */
    [[nodiscard]] operator entt::entity() const;

    /**
     * @brief Checks validity via bool conversion.
     *
     * @return true if the entity is valid
     */
    [[nodiscard]] operator bool() const;

    /**
     * @brief Compares two entities for equality.
     *
     * @param other The entity to compare against
     * @return true if both entities refer to the same underlying entity
     */
    [[nodiscard]] bool operator==(const Entity& other) const;

    /**
     * @brief Returns this entity's parent.
     *
     * @return The parent entity, or an invalid entity if no parent exists
     */
    [[nodiscard]] Entity get_parent() const;

    /**
     * @brief Returns the raw EnTT ID of this entity's parent.
     *
     * @return The parent's entity ID, or entt::null if no parent
     */
    [[nodiscard]] entt::entity get_parent_id() const;

    /**
     * @brief Returns a range of direct children.
     *
     * Provides an iterable range over this entity's immediate children.
     * Uses ChildIterator which traverses the sibling linked list.
     *
     * @return ChildRange for use with range-based for loops
     *
     * @par Example:
     * @code
     * for (auto child : entity.children()) {
     *     child.get_component<TransformComponent>().update();
     * }
     * @endcode
     *
     * @see descendants() for recursive traversal
     */
    [[nodiscard]] ChildRange children() const;

    /**
     * @brief Returns a range of all descendants (recursive).
     *
     * Provides an iterable range over this entity's entire subtree using
     * depth-first traversal. Uses RecursiveChildIterator with internal stack.
     *
     * @return RecursiveChildRange for use with range-based for loops
     *
     * @par Example:
     * @code
     * // Disable all entities in the subtree
     * for (auto descendant : entity.descendants()) {
     *     descendant.add_component<DisabledTag>();
     * }
     * @endcode
     *
     * @see children() for direct children only
     */
    [[nodiscard]] RecursiveChildRange descendants() const;

    /**
     * @brief Checks if this entity is an ancestor of another.
     *
     * @param other The potential descendant
     * @return true if other is a descendant of this entity
     */
    [[nodiscard]] bool is_ancestor_of(Entity other) const;

    /**
     * @brief Checks if this entity is a descendant of another.
     *
     * @param other The potential ancestor
     * @return true if this entity is a descendant of other
     */
    [[nodiscard]] bool is_descendant_of(Entity other) const;

    /**
     * @brief Returns the registry containing this entity.
     *
     * @return Reference to the entt::registry
     */
    [[nodiscard]] entt::registry& get_registry() const;

private:
    entt::handle handle{};

    inline const static auto no_name = STRING_ID("Unnamed");
};

static const Entity null_entity{};
} // portal
