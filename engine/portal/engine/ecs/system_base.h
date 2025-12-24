//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "entity.h"
#include "portal/core/type_traits.h"
#include "portal/core/jobs/job.h"

namespace portal
{
struct FrameContext;
}

namespace portal::ecs
{
class SystemBase;
class Registry;

/**
 * @brief Execution policy for systems.
 *
 * Determines how a system's execute() method is dispatched. The policy can be changed
 * at runtime via SystemBase::set_policy().
 */
enum class ExecutionPolicy
{
    Sequential,
    Parallel,
};

/**
 * @brief Concept for systems that implement the on_component_added callback.
 *
 * Systems satisfying this concept can react when a component is constructed on an entity.
 * The callback is automatically registered when the system is registered with the registry.
 *
 * @tparam System The system type
 * @tparam Component The component type
 *
 * @par Example:
 * @code
 * class MySystem : public System<MySystem, Owns<MyComponent>> {
 *     void on_component_added(Entity entity, MyComponent& component) {
 *         // React to component addition
 *     }
 * };
 * @endcode
 */
template <typename System, typename Component>
concept OnComponentAdded = requires(System& system, Entity entity, Component& component) {
    { system.on_component_added(entity, component) } -> std::same_as<void>;
};

/**
 * @brief Concept for systems that implement the on_component_removed callback.
 *
 * Systems satisfying this concept can react when a component is destroyed on an entity.
 *
 * @tparam System The system type
 * @tparam Component The component type
 */
template <typename System, typename Component>
concept OnComponentRemoved = requires(System& system, Entity entity, Component& component) {
    { system.on_component_removed(entity, component) } -> std::same_as<void>;
};

/**
 * @brief Concept for systems that implement the on_component_changed callback.
 *
 * Systems satisfying this concept can react when a component is modified via patch_component().
 *
 * @tparam System The system type
 * @tparam Component The component type
 */
template <typename System, typename Component>
concept OnComponentChanged = requires(System& system, Entity entity, Component& component) {
    { system.on_component_changed(entity, component) } -> std::same_as<void>;
};

/**
 * @brief Concept for systems with sequential execute(Registry&).
 * @tparam System The system type
 */
template <typename System>
concept HasExecute = requires(System& system, Registry& registry) {
    { system.execute(registry) } -> std::same_as<void>;
};

/**
 * @brief Concept for systems with sequential execute(FrameContext&, Registry&).
 * @tparam System The system type
 */
template <typename System>
concept HasExecuteWithContext = requires(System& system, FrameContext& context, Registry& registry) {
    { system.execute(context, registry) } -> std::same_as<void>;
};

/**
 * @brief Concept for systems that can execute sequentially.
 *
 * Systems must implement either execute(Registry&) or execute(FrameContext&, Registry&).
 *
 * @tparam System The system type
 */
template <typename System>
concept SequentialExecution = HasExecute<System> || HasExecuteWithContext<System>;

/**
 * @brief Concept for systems with parallel execute(Registry&, Scheduler&).
 * @tparam System The system type
 */
template <typename System>
concept HasExecuteJob = requires(System& system, Registry& registry, jobs::Scheduler& scheduler) {
    { system.execute(registry, scheduler) } -> std::same_as<Job<>>;
};

/**
 * @brief Concept for systems with parallel execute(Registry&, Scheduler&, Counter*).
 * @tparam System The system type
 */
template <typename System>
concept HasExecuteJobCounter = requires(System& system, Registry& registry, jobs::Scheduler& scheduler, jobs::Counter* counter) {
    { system.execute(registry, scheduler, counter) } -> std::same_as<Job<>>;
};

/**
 * @brief Concept for systems with parallel execute(FrameContext&, Registry&, Scheduler&).
 * @tparam System The system type
 */
template <typename System>
concept HasExecuteJobWithContext = requires(System& system, FrameContext& context, Registry& registry, jobs::Scheduler& scheduler) {
    { system.execute(context, registry, scheduler) } -> std::same_as<Job<>>;
};

/**
 * @brief Concept for systems with parallel execute(FrameContext&, Registry&, Scheduler&, Counter*).
 * @tparam System The system type
 */
template <typename System>
concept HasExecuteJobCounterWithContext = requires(System& system, FrameContext& context, Registry& registry, jobs::Scheduler& scheduler, jobs::Counter* counter) {
    { system.execute(context, registry, scheduler, counter) } -> std::same_as<Job<>>;
};

/**
 * @brief Concept for systems that can execute in parallel.
 *
 * Systems must implement an execute() method that returns Job<> and accepts a Scheduler.
 * Supports variants with/without FrameContext and Counter parameters.
 *
 * @tparam System The system type
 */
template <typename System>
concept ParallelExecution = HasExecuteJob<System> || HasExecuteJobWithContext<System> || HasExecuteJobCounter<System> || HasExecuteJobCounterWithContext<System>;

/**
 * @brief Concept enforcing the system interface contract.
 *
 * Systems must satisfy either SequentialExecution or ParallelExecution, and must provide
 * register_to(Registry&) and get_name() methods.
 *
 * @tparam System The system type
 *
 * @par Required Methods:
 * - `static StringId get_name()` - Returns system identifier
 * - `void register_to(Registry&)` - Registers the system
 * - Either Sequential or Parallel execute() signature
 */
template <typename System>
concept SystemConcept =
    (SequentialExecution<System> || ParallelExecution<System>) &&
    requires(System& system, Registry& registry) {
        { system.register_to(registry) } -> std::same_as<void>;
        { system.get_name() } -> std::same_as<StringId>;
    };

/**
 * @brief Component ownership wrapper for owned components.
 *
 * Wraps a component type to indicate that a system "owns" it for iteration purposes.
 * EnTT packs owned components together in memory for cache-efficient access.
 *
 * @tparam C The component type
 *
 * @par Example:
 * @code
 * class MySystem : public System<
 *     MySystem,
 *     Owns<TransformComponent>,  // Owned (cache-optimized)
 *     Views<RenderComponent>     // Viewed (not optimized)
 * > {
 *     // System definition
 * };
 * @endcode
 *
 * @see Views for viewed component wrapper
 */
template <typename C>
struct Owns
{
    using comp = C;
};

/**
 * @brief Component ownership wrapper for viewed components.
 *
 * Wraps a component type to indicate that a system "views" it but doesn't own it.
 * EnTT includes viewed components in queries but doesn't optimize their memory layout.
 *
 * @tparam C The component type
 *
 * @see Owns for owned component wrapper
 */
template <typename C>
struct Views
{
    using comp = C;
    static constexpr auto comp_view = entt::get<C>;
};

/**
 * @brief Concept for Views<T> specializations.
 * @tparam C The wrapped type
 */
template <typename C>
concept ComponentView = is_specialization_of_v<C, Views>;

/**
 * @brief Concept for Owns<T> specializations.
 * @tparam C The wrapped type
 */
template <typename C>
concept ComponentOwned = is_specialization_of_v<C, Owns>;

/**
 * @brief Concept for component ownership wrappers (Owns<T> or Views<T>).
 * @tparam C The wrapped type
 */
template <typename C>
concept ComponentOwnership = ComponentView<C> || ComponentOwned<C>;

/**
 * @brief Base class for all ECS systems.
 *
 * Provides execution policy management that derived systems inherit.
 * The policy determines whether execute() runs sequentially or is dispatched as a job.
 *
 * @see System for the CRTP-based system template
 * @see ExecutionPolicy for policy options
 */
class SystemBase
{
public:
    /**
     * @brief Constructs a system base with the specified execution policy.
     *
     * @param policy The execution policy. Defaults to Sequential.
     */
    explicit SystemBase(
        const ExecutionPolicy policy = ExecutionPolicy::Sequential
    ) : policy(policy) {}

    virtual ~SystemBase() = default;

    /**
     * @brief Changes the system's execution policy at runtime.
     *
     * @param new_policy The new execution policy
     *
     * @par Example:
     * @code
     * my_system.set_policy(ExecutionPolicy::Parallel);
     * @endcode
     */
    void set_policy(const ExecutionPolicy new_policy) { policy = new_policy; }

    /**
     * @brief Returns the current execution policy.
     *
     * @return The execution policy
     */
    [[nodiscard]] ExecutionPolicy get_policy() const { return policy; }

protected:
    ExecutionPolicy policy;
};
}
