//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <entt/entity/registry.hpp>

#include "entity.h"
#include "system_base.h"
#include "portal/application/modules/module.h"
#include "portal/engine/components/base.h"

namespace portal::ecs
{
/**
 * @brief Central registry for all entity and component operations in the Portal ECS.
 *
 * The Registry class wraps EnTT's registry and provides Portal-specific patterns and
 * conventions for entity management. It serves as the source of truth for all
 * entity creation, component storage, and system registration.
 *
 * Key responsibilities:
 * - Entity lifecycle management (creation, destruction, lookup)
 * - Component storage and access
 * - System registration and management
 * - Default component configuration (components attached to all entities)
 * - Parent-child entity hierarchy support
 *
 * The Registry automatically attaches default components to all created entities:
 * - RelationshipComponent (for parent-child hierarchies)
 * - TransformComponent (for spatial positioning)
 *
 * @par Example Usage:
 * @code
 * Registry registry;
 *
 * // Create a top-level entity
 * auto player = registry.create_entity(STRING_ID("player"));
 * player.add_component<HealthComponent>(100.0f);
 *
 * // Create a child entity (e.g., weapon attached to player)
 * auto weapon = registry.create_child_entity(player, STRING_ID("sword"));
 *
 * // Register a system
 * auto& physics_system = registry.register_system<PhysicsSystem>();
 *
 * // Access components via views
 * for (auto entity : registry.view<TransformComponent, RenderComponent>()) {
 *     auto& transform = entity.get_component<TransformComponent>();
 *     // Process entity...
 * }
 * @endcode
 *
 * @note All entity creation methods ensure default components are attached automatically.
 * @note The Registry maintains an "env" entity for global/environmental state.
 *
 * @see Entity for component access and manipulation
 * @see System for defining game logic that operates on entities
 */
class Registry final : public Module<>
{
public:
    constexpr static auto ENV_ENTITY_ID = "env";

    /**
     * @brief Constructs the Registry and initializes the ECS.
     *
     * Creates the environment entity and configures default components
     * (RelationshipComponent, TransformComponent) to be attached to all
     * subsequently created entities.
     */
    Registry(ModuleStack& stack);

    /**
     * @brief Destroys the Registry and all associated entities.
     */
    ~Registry() override;

    /**
     * @brief Wraps a raw EnTT entity ID in a Portal Entity handle.
     *
     * @param id The raw EnTT entity identifier
     * @return Entity wrapper providing Portal's component access API
     *
     * @note This method does not validate that the entity exists.
     */
    Entity entity_from_id(entt::entity id);

    /**
     * @brief Finds an entity by name, or creates it if it doesn't exist.
     *
     * Searches for an entity with the specified name in the registry. If found,
     * returns the existing entity. If not found, creates a new top-level entity
     * with that name.
     *
     * @param entity_name The name for the entity
     * @return The found or newly created entity
     *
     * @note This creates a top-level entity (no parent) if creation is needed.
     * @see find_or_create_child for scoped creation under a parent
     */
    Entity find_or_create(const StringId& entity_name);

    /**
     * @brief Find an entity by name.
     *
     * Searches for an entity with the specified name in the registry.
     *
     * @param entity_name The name for the entity.
     * @reaturn The entity if found, std::nullopt otherwise
     */
    std::optional<Entity> find_by_name(const StringId& entity_name);

    /**
     * @brief Creates a new top-level entity.
     *
     * Creates an entity with no default components, only a name component if a name is provided
     *
     * @param name Optional unique identifier for the entity (defaults to INVALID_STRING_ID)
     * @param components An optional list of additional components to add to the entity
     * @return The newly created entity
     *
     * @par Example:
     * @code
     * auto player = registry.create_entity(STRING_ID("player"));
     * auto unnamed = registry.create_entity(); // Anonymous entity
     * @endcode
     */
    template <typename... Components>
    Entity create_entity(const StringId& name = INVALID_STRING_ID, Components&&... components)
    {
        if (name != INVALID_STRING_ID)
            return make_entity<Components...>(NameComponent{name}, std::forward<Components>(components)...);
        return make_entity<Components...>(std::forward<Components>(components)...);
    }


    /**
     * @brief Finds a child entity by name, or creates it if it doesn't exist.
     *
     * Searches for an entity with the specified name among the children of the
     * given parent. If found, returns the existing child. If not found, creates
     * a new child entity with that name.
     *
     * @param parent The parent entity to search under
     * @param entity_name The unique identifier for the child entity
     * @return The found or newly created child entity
     *
     * @note The search is scoped to direct children only, not descendants.
     */
    Entity find_or_create_child(Entity parent, const StringId& entity_name);

    /**
     * @brief Creates a new entity as a child of the specified parent.
     *
     * Creates an entity and establishes it as a child in the parent-child
     * hierarchy. The entity receives default components and is automatically
     * linked into the parent's child list via RelationshipComponent.
     *
     * @param parent The parent entity
     * @param entity_name Optional unique identifier for the child entity
     * @return The newly created child entity
     *
     * @par Example:
     * @code
     * auto player = registry.create_entity(STRING_ID("player"));
     * auto weapon = registry.create_child_entity(player, STRING_ID("sword"));
     * auto shield = registry.create_child_entity(player, STRING_ID("shield"));
     * @endcode
     *
     * @see Entity::set_parent for reparenting existing entities
     */
    Entity create_child_entity(Entity parent, const StringId& entity_name = INVALID_STRING_ID);

    /**
     * @brief Returns the special environment entity.
     *
     * The environment entity is a special entity created during Registry construction
     * that holds global or environmental state. It's useful for singleton components
     * that don't belong to any particular game entity.
     *
     * @return The environment entity
     *
     * @par Example:
     * @code
     * auto env = registry.get_env_entity();
     * env.add_component<SomeGlobalComponent>(settings);
     * @endcode
     */
    [[nodiscard]] Entity get_env_entity() const;

    /**
     * @brief Destroys an entity and optionally its children.
     *
     * Removes the entity from the registry, destroying all its components and
     * breaking its relationships with parent and siblings. By default, all
     * children are also destroyed recursively.
     *
     * @param entity The entity to destroy
     * @param exclude_children If true, children are orphaned but not destroyed.
     *                         If false (default), children are destroyed recursively.
     *
     */
    void destroy_entity(Entity entity, bool exclude_children = false);

    /**
     * @brief Clears the entire registy
     *
     * Destroys all entites and components from the registry
     */
    void clear();

    /**
     * @brief Removes all instances of a component type from the registry.
     *
     * Destroys all components of the specified type across all entities.
     * This is useful for bulk cleanup or resetting specific component pools.
     *
     * @tparam C The component type to clear
     *
     * @par Example:
     * @code
     * // Remove all physics bodies from all entities
     * registry.clear<PhysicsBodyComponent>();
     * @endcode
     *
     * @note This does not destroy the entities themselves, only the specified component.
     */
    template <typename C>
    void clear()
    {
        registry.clear<C>();
    }

    /**
     * @brief Creates a view for iterating entities with specific components.
     *
     * Returns a lazy-evaluated view that iterates all entities possessing all
     * specified component types. Views are not cached and have minimal overhead.
     *
     * @tparam T Component types to include in the view
     * @return A range of Entity objects that can be used with range-based for loops
     *
     * @par Example:
     * @code
     * // Iterate all entities with both Transform and Render components
     * for (auto entity : registry.view<TransformComponent, RenderComponent>()) {
     *     auto& transform = entity.get_component<TransformComponent>();
     *     auto& render = entity.get_component<RenderComponent>();
     *     // Process entity...
     * }
     * @endcode
     *
     * @note Views are lightweight and can be created on-demand each frame.
     * @note For performance-critical iteration, consider using groups instead.
     * @see group for cached, optimized iteration
     */
    template <typename... T>
    auto view() const
    {
        return view_raw<T...>() | std::views::transform([this](const auto entity) { return Entity{entity, const_cast<entt::registry&>(registry)}; });
    }

    /**
     * @brief Creates an EnTT group for optimized component iteration.
     *
     * Returns a group that provides cache-friendly iteration over entities with
     * the specified components. Groups maintain sorted storage for maximum
     * performance and are cached internally by EnTT.
     *
     * @tparam T Component types to include in the group
     * @return An EnTT group object
     *
     * @note Groups are more efficient than views for frequent iteration.
     * @note This overload does not distinguish between owned and viewed components.
     * @see group(V&&...) for owned/viewed component distinction
     */
    template <typename... T>
    auto group()
    {
        return registry.group<T...>();
    }

    /**
     * @brief Creates an EnTT group with owned and viewed component distinction.
     *
     * Returns a group that optimizes storage layout based on component ownership
     * semantics. Owned components are packed together for cache locality, while
     * viewed components are accessed via indirection.
     *
     * @tparam T Owned component types (drive iteration, packed storage)
     * @tparam V Viewed component types (accessed but not optimized)
     * @param views Views<Component> wrappers for the viewed components
     * @return An EnTT group object optimized for the specified ownership pattern
     *
     * @par Example:
     * @code
     * // Group with Transform owned, Render viewed
     * auto grp = registry.group<TransformComponent>(Views<RenderComponent>{});
     * for (auto entity : grp) {
     *     // Efficient iteration over transforms, with render access
     * }
     * @endcode
     *
     * @note This is typically called automatically by System::group().
     * @see System for how systems use owned/viewed semantics
     */
    template <typename... T, ComponentView... V>
    auto group(V&&... views)
    {
        return registry.group<T...>((views.comp_view)...);
    }

    template <typename... T, typename... V>
    auto group(V&&... views)
    {
        return registry.group<T...>((views)...);
    }

    /**
     * @brief Registers a component to be added to all created entities.
     *
     * Configures the registry to automatically attach the specified component type
     * to every entity created after this call. Arguments are forwarded to the
     * component's constructor.
     *
     * @tparam C The component type to add by default
     * @tparam Args Argument types for the component constructor
     * @param args Arguments forwarded to the component constructor
     *
     * @par Example:
     * @code
     * // Make all entities receive a HealthComponent with 100 HP
     * registry.add_default_component<HealthComponent>(100.0f);
     *
     * // All entities created after this point will have HealthComponent
     * auto player = registry.create_entity(STRING_ID("player"));
     * assert(player.has_component<HealthComponent>());
     * @endcode
     *
     * @note This only applies to entities created after this function is called.
     * @note By default, Registry already adds RelationshipComponent and TransformComponent.
     */
    template <typename C, typename... Args>
    void add_default_component(Args&&... args)
    {
        registry.on_construct<entt::entity>().connect<&entt::registry::emplace_or_replace<C>>(std::forward<Args>(args)...);
    }

    /**
     * @brief Constructs and registers a system with the registry.
     *
     * Creates a new system instance, registers it with the registry, and returns the system by value.
     *
     * @tparam S The system type (must satisfy SystemConcept)
     * @tparam Args Argument types for the system constructor
     * @param args Arguments forwarded to the system constructor
     * @return The newly constructed and registered system
     *
     * @par Example:
     * @code
     * // Register a physics system with Sequential execution
     * auto physics = registry.register_system<PhysicsSystem>(ExecutionPolicy::Sequential);
     *
     * // Register a system with custom parameters
     * auto ai = registry.register_system<AISystem>(difficulty_level);
     * @endcode
     *
     * @note The system's register_to() method is called automatically.
     * @see register_system(S&) for registering existing system instances
     */
    template <SystemConcept S, typename... Args>
    S register_system(Args&&... args)
    {
        S system(std::forward<Args>(args)...);
        register_system(system);
        return system;
    }

    /**
     * @brief Registers an existing system instance with the registry.
     *
     * Calls the system's register_to() method, which establishes the system's
     * EnTT group for optimized iteration and connects component lifecycle callbacks.
     *
     * @tparam S The system type (must satisfy SystemConcept)
     * @param system Reference to the system to register
     *
     * @par Example:
     * @code
     * MySystem system(ExecutionPolicy::Parallel);
     * registry.register_system(system);
     * // System is now registered and ready to execute
     * @endcode
     *
     * @note This is called automatically by register_system(Args&&...).
     */
    template <SystemConcept S>
    void register_system(S& system)
    {
        system.register_to(registry);
    }

    /**
     * @brief Returns the underlying EnTT registry.
     *
     * Provides direct access to the wrapped entt::registry for advanced use cases
     * that require EnTT-specific functionality not exposed by the Portal API.
     *
     * @return Reference to the internal entt::registry
     *
     * @note Use this only when Portal's API is insufficient.
     * @note Prefer Portal's Entity and Registry methods for typical usage.
     */
    [[nodiscard]] entt::registry& get_raw_registry() { return registry; }

protected:
    template <typename... Components>
    Entity make_entity(Components&&... components)
    {
        PORTAL_PROF_ZONE();

        auto entity = Entity{registry.create(), registry};
        (entity.add_component<Components>(std::forward<Components>(components)), ...);
        return entity;
    }

private:
    template <typename... T>
    auto view_raw() const
    {
        return registry.view<T...>();
    }

private:
    entt::registry registry;
    entt::entity env_entity;
};
} // portal
