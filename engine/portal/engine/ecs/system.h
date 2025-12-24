//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <entt/entt.hpp>

#include "entity.h"
#include "registry.h"
#include "system_base.h"

#include "portal/core/type_traits.h"
#include "portal/core/debug/assert.h"
#include "portal/core/jobs/scheduler.h"
#include "portal/core/strings/string_id.h"

namespace portal
{
struct FrameContext;

namespace jobs
{
    class Scheduler;
}
}

namespace portal::ecs
{
/**
 * @brief CRTP base class for ECS systems that operate on entities with specific components.
 *
 * System provides a template-based framework for defining game logic that operates on
 * entities possessing certain components. It uses the Curiously Recurring Template Pattern
 * (CRTP) to achieve static polymorphism without virtual dispatch overhead.
 *
 * The class template accepts a Derived type (the system implementation) and a pack of
 * ComponentOwnership wrappers (Owns<T> or Views<T>) that specify which components the
 * system operates on and their access semantics.
 *
 * @tparam Derived The derived system class (CRTP parameter)
 * @tparam Components Pack of Owns<T> or Views<T> wrappers specifying component ownership
 *
 * @par Component Ownership:
 * - **Owns\<T\>**: EnTT packs these components for cache-efficient iteration
 * - **Views\<T\>**: Components accessed but not layout-optimized
 *
 * @par Execution Policies:
 * - **Sequential**: `void execute(Registry&)` or `void execute(FrameContext&, Registry&)`
 * - **Parallel**: `Job<> execute(Registry&, Scheduler&)` or variants with FrameContext/Counter
 *
 * @par Example - Sequential System:
 * @code
 * class MySystem : public System<MySystem, Owns<TransformComponent>> {
 * public:
 *     MySystem() : System(ExecutionPolicy::Sequential) {}
 *     static StringId get_name() { return STRING_ID("MySystem"); }
 *
 *     void execute(Registry& registry) {
 *         for (auto [entity, transform] : group(registry).each()) {
 *             // Process entities
 *         }
 *     }
 * };
 * @endcode
 *
 * @see SystemBase for execution policy management
 * @see Owns for owned component wrapper
 * @see Views for viewed component wrapper
 */
template <typename Derived, ComponentOwnership... Components>
class System : public SystemBase
{
public:
    /**
     * @brief Constructs a system with the specified execution policy.
     *
     * @param policy The execution policy (Sequential or Parallel). Defaults to Sequential.
     */
    explicit System(
        const ExecutionPolicy policy = ExecutionPolicy::Sequential
    ) : SystemBase(policy)
    {
        static_assert(
            ecs::SystemConcept<Derived>,
            "System<Derived,...>: Derived must implement get_name() and execute() or execute_single(entity)."
        );
    }

    /**
     * @brief Registers the system with the registry.
     *
     * Sets up the system's EnTT group for optimized component iteration and registers
     * component lifecycle callbacks if the derived class implements them.
     *
     * @param registry The registry to register with
     *
     * @note Called automatically by Registry::register_system().
     */
    void register_to(Registry& registry)
    {
        register_component_callbacks(registry);
        // We call it once for underlying storage optimization
        group(registry);
    }

    /**
     * @brief Internal execution dispatcher (called by system orchestrator).
     *
     * Dispatches to the derived system's execute() method based on the current execution
     * policy and detected method signatures. Handles both sequential and parallel execution,
     * with automatic wrapping of sequential systems as jobs when running in parallel mode.
     *
     * @param context Frame timing and resource context
     * @param registry The ECS registry
     * @param scheduler Job scheduler for parallel execution
     * @param counter Optional job counter for synchronization
     *
     * @note This is an internal method called by the system orchestrator. do not call it yourself.
     */
    void _execute(FrameContext& context, Registry& registry, jobs::Scheduler& scheduler, jobs::Counter* counter)
    {
        // TODO: move the policy to compile time argument for performance
        switch (policy)
        {
            using enum ExecutionPolicy;

        case Sequential:
            {
                if constexpr (ecs::SequentialExecution<Derived>)
                {
                    if constexpr (ecs::HasExecuteWithContext<Derived>)
                        derived().execute(context, registry);
                    else
                        derived().execute(registry);
                }
                else
                {
                    PORTAL_ASSERT(false, "Cannot run parallel execute with sequential policy");
                }
                break;
            }
        case Parallel:
            {
                if constexpr (ecs::ParallelExecution<Derived>)
                {
                    if constexpr (ecs::HasExecuteJobWithContext<Derived>)
                        scheduler.dispatch_job<void>(
                            derived().execute(context, registry, scheduler),
                            JobPriority::Normal,
                            counter
                        );
                    else if constexpr (ecs::HasExecuteJobCounterWithContext<Derived>)
                        scheduler.dispatch_job<void>(
                            derived().execute(context, registry, scheduler, counter),
                            JobPriority::Normal,
                            counter
                        );
                    else if constexpr (ecs::HasExecuteJob<Derived>)
                        scheduler.dispatch_job<void>(
                            derived().execute(registry, scheduler),
                            JobPriority::Normal,
                            counter
                        );
                    else
                        scheduler.dispatch_job<void>(
                            derived().execute(registry, scheduler, counter),
                            JobPriority::Normal,
                            counter
                        );
                }
                else if constexpr (ecs::SequentialExecution<Derived>)
                {
                    scheduler.dispatch_job<void>(
                        [this, &registry, &context]() -> Job<>
                        {
                            if constexpr (ecs::HasExecuteWithContext<Derived>)
                            {
                                derived().execute(context, registry);
                            }
                            else
                            {
                                // Cannot mark [[maybe_unused]] on lambda capture :(
                                std::ignore = context;
                                derived().execute(registry);
                            }
                            co_return;
                        }(),
                        JobPriority::Normal,
                        counter
                    );
                }
                else
                {
                    PORTAL_ASSERT(false, "Invalid execution policy");
                }
                break;
            }
        }
    }

protected:
    /**
     * @brief Creates an EnTT group for iterating entities with the system's components.
     *
     * Returns a cached EnTT group that provides cache-friendly iteration over entities
     * possessing all components specified in the system's template parameters. The group
     * is created based on the component ownership semantics (Owns vs Views).
     *
     * Components wrapped in Owns<T> are treated as owned (packed together for cache locality),
     * while components wrapped in Views<T> are treated as viewed (accessed via indirection).
     *
     * @param registry The registry to create the group from
     * @return EnTT group object for iteration
     *
     * @par Example:
     * @code
     * class MySystem : public System<
     *     MySystem,
     *     Owns<TransformComponent>,
     *     Views<RenderComponent>
     * > {
     *     void execute(Registry& registry) {
     *         // group() returns entities with both components
     *         for (auto [entity, transform, render] : group(registry).each()) {
     *             // TransformComponent access is cache-optimized
     *             // RenderComponent access may be slower
     *         }
     *     }
     * };
     * @endcode
     *
     * @note The group is cached by EnTT - subsequent calls return the same optimized view.
     * @note This method is called automatically by register_to() to set up storage optimization.
     */
    static auto group(Registry& registry)
    {
        // TODO: decorate with custom `group` class that transforms `entt::entity` to `Entity` class
        using owned_list = filter_t<is_owns_wrapper, Components...>;
        using viewed_list = filter_t<is_views_wrapper, Components...>;
        return group_caller<owned_list, viewed_list>::call(registry);
    }

private:
    template <typename T>
    struct is_owns_wrapper : std::bool_constant<is_specialization_of_v<T, ecs::Owns>> {};

    template <typename T>
    struct is_views_wrapper : std::bool_constant<is_specialization_of_v<T, ecs::Views>> {};

    template <typename Owned, typename Viewed>
    struct group_caller;

    template <typename... Owned, typename... Viewed>
    struct group_caller<type_list<Owned...>, type_list<Viewed...>>
    {
        static auto call(Registry& registry)
        {
            if constexpr (sizeof...(Viewed) > 0)
                return registry.get_raw_registry().group<typename Owned::comp...>(entt::get<typename Viewed::comp...>);
            else
                return registry.get_raw_registry().group<typename Owned::comp...>();
        }
    };

    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

    template <typename Component>
    void on_construct(entt::registry& registry, const entt::entity entity_raw)
    {
        auto entity = Entity(entity_raw, registry);
        auto& component = entity.get_component<Component>();
        derived().on_component_added(entity, component);
    }

    template <typename Component>
    void on_destroy(entt::registry& registry, const entt::entity entity_raw)
    {
        auto entity = Entity(entity_raw, registry);
        auto& component = entity.get_component<Component>();
        derived().on_component_removed(entity, component);
    }

    template <typename Component>
    void on_update(entt::registry& registry, const entt::entity entity_raw)
    {
        auto entity = Entity(entity_raw, registry);
        auto& component = entity.get_component<Component>();
        derived().on_component_changed(entity, component);
    }

    template <typename Component>
    void register_component_callbacks_single(Registry& registry)
    {
        auto& raw_registry = registry.get_raw_registry();
        if constexpr (ecs::OnComponentAdded<Derived, Component>)
        {
            raw_registry.on_construct<Component>().template connect<&System::on_construct<Component>>(this);
        }

        if constexpr (ecs::OnComponentRemoved<Derived, Component>)
        {
            raw_registry.on_destroy<Component>().template connect<&System::on_destroy<Component>>(this);
        }

        if constexpr (ecs::OnComponentChanged<Derived, Component>)
        {
            raw_registry.on_update<Component>().template connect<&System::on_update<Component>>(this);
        }
    }

    void register_component_callbacks(Registry& registry)
    {
        (register_component_callbacks_single<typename Components::comp>(registry), ...);
    }

protected:
    StringId name;
};
}
