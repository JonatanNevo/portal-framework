//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "entity.h"
#include "portal/core/type_traits.h"

namespace portal
{
struct FrameContext;
}

namespace portal::ecs
{
class SystemBase;
class Registry;


enum class ExecutionPolicy
{
    Sequential,
    Parallel,
};

template <typename System, typename Component>
concept OnComponentAdded = requires(System& system, Entity entity, Component& component) {
    { system.on_component_added(entity, component) } -> std::same_as<void>;
};

template <typename System, typename Component>
concept OnComponentRemoved = requires(System& system, Entity entity, Component& component) {
    { system.on_component_removed(entity, component) } -> std::same_as<void>;
};

template <typename System, typename Component>
concept OnComponentChanged = requires(System& system, Entity entity, Component& component) {
    { system.on_component_changed(entity, component) } -> std::same_as<void>;
};

template <typename System>
concept HasExecute = requires(System& system, Registry& registry) {
    { system.execute(registry) } -> std::same_as<void>;
};

template <typename System>
concept HasExecuteWithContext = requires(System& system, FrameContext& context, Registry& registry) {
    { system.execute(context, registry) } -> std::same_as<void>;
};

template <typename System>
concept SequentialExecution = HasExecute<System> || HasExecuteWithContext<System>;

template <typename System>
concept HasExecuteSingle = requires(System& system, Registry& registry, Entity entity) {
    { system.execute_single(registry, entity) } -> std::same_as<void>;
};

template <typename System>
concept HasExecuteSingleWithContext = requires(System& system, FrameContext& context, Registry& registry, Entity entity) {
    { system.execute_single(context, registry, entity) } -> std::same_as<void>;
};

template <typename System>
concept ParallelExecution = HasExecuteSingle<System> || HasExecuteSingleWithContext<System>;

template <typename System>
concept SystemConcept =
    (SequentialExecution<System> || ParallelExecution<System>) &&
    requires(System& system, Registry& registry) {
        { system.register_to(registry) } -> std::same_as<void>;
        { system.get_name() } -> std::same_as<StringId>;
    };

template <typename C>
struct Owns
{
    using comp = C;
};

template <typename C>
struct Views
{
    using comp = C;
    static constexpr auto comp_view = entt::get<C>;
};

template <typename C>
concept ComponentView = is_specialization_of_v<C, Views>;

template <typename C>
concept ComponentOwned = is_specialization_of_v<C, Owns>;

template <typename C>
concept ComponentOwnership = ComponentView<C> || ComponentOwned<C>;

class SystemBase
{
public:
    explicit SystemBase(
        const ExecutionPolicy policy = ExecutionPolicy::Sequential
    ) : policy(policy) {}

    virtual ~SystemBase() = default;

    void set_policy(const ExecutionPolicy new_policy) { policy = new_policy; }
    [[nodiscard]] ExecutionPolicy get_policy() const { return policy; }

protected:
    ExecutionPolicy policy;
};
}
