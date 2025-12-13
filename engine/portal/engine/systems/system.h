//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <entt/entt.hpp>

#include "portal/core/debug/assert.h"
#include "portal/core/jobs/scheduler.h"
#include "portal/core/strings/string_id.h"

namespace portal
{
namespace jobs
{
    class Scheduler;
}

class SystemBase;

namespace ecs
{
    enum class ExecutionPolicy
    {
        Sequential,
        Parallel,
    };

    template <typename System, typename Component>
    concept OnComponentAdded = requires(System& system, entt::entity entity, Component& component) {
        { system.on_component_added(entity, component) } -> std::same_as<void>;
    };

    template <typename System, typename Component>
    concept OnComponentRemoved = requires(System& system, entt::entity entity, Component& component) {
        { system.on_component_removed(entity, component) } -> std::same_as<void>;
    };

    template <typename System, typename Component>
    concept OnComponentChanged = requires(System& system, entt::entity entity, Component& component) {
        { system.on_component_changed(entity, component) } -> std::same_as<void>;
    };

    template <typename System>
    concept SequentialExecution = requires(System& system) {
        { system.execute() } -> std::same_as<void>;
    };

    template <typename System>
    concept ParallelExecution = requires(System& system, entt::entity entity) {
        { system.execute_single(entity) } -> std::same_as<void>;
    };

    template <typename System>
    concept SystemConcept =
        (SequentialExecution<System> || ParallelExecution<System>) &&
        requires(System& system) {
            { system.get_name() } -> std::same_as<StringId>;
        };
}


class SystemBase
{
public:
    explicit SystemBase(
        entt::registry& registry,
        jobs::Scheduler& scheduler,
        const ecs::ExecutionPolicy policy = ecs::ExecutionPolicy::Sequential
    ) : policy(policy), registry(registry), scheduler(scheduler) {}

    void set_policy(const ecs::ExecutionPolicy new_policy) { policy = new_policy; }
    [[nodiscard]] ecs::ExecutionPolicy get_policy() const { return policy; }

protected:
    ecs::ExecutionPolicy policy;
    entt::registry& registry;
    jobs::Scheduler& scheduler;
};


template <typename Derived, typename... Components>
class System : public SystemBase
{
public:
    explicit System(
        entt::registry& registry,
        jobs::Scheduler& scheduler,
        const ecs::ExecutionPolicy policy = ecs::ExecutionPolicy::Sequential
    ) : SystemBase(registry, scheduler, policy)
    {
        static_assert(
            ecs::SystemConcept<Derived>,
            "System<Derived,...>: Derived must implement get_name() and execute() or execute_single(entity)."
        );

        register_component_callbacks();
        // We call it once for underlying storage optimization
        group();
    }

    void _execute()
    {
        switch (policy)
        {
            using enum ecs::ExecutionPolicy;

        case Sequential:
            {
                if constexpr (ecs::SequentialExecution<Derived>)
                    derived().execute();
                else
                    PORTAL_ASSERT(false, "Cannot run parallel execute with sequential policy");
                break;
            }
        case Parallel:
            {
                if constexpr (ecs::ParallelExecution<Derived>)
                    scheduler.dispatch_jobs(
                        group() | std::views::transform(
                            [this](auto entity)
                            {
                                return [this, entity]() -> Job<>
                                {
                                    derived().execute_single(entity);
                                    co_return;
                                };
                            }
                        )
                    );
                else if constexpr (ecs::SequentialExecution<Derived>)
                    scheduler.dispatch_job(
                        [this]() -> Job<>
                        {
                            derived().execute();
                            co_return;
                        }
                    );
                else
                    PORTAL_ASSERT(false, "Invalid execution policy");
                break;
            }
        }
    }

protected:
    auto group() const
    {
        return registry.group<Components...>();
    }

private:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

    template <typename Component>
    void on_construct(entt::registry& registry, entt::entity entity)
    {
        auto& component = registry.get<Component>(entity);
        derived().on_component_added(entity, component);
    }

    template <typename Component>
    void on_destroy(entt::registry& registry, entt::entity entity)
    {
        auto& component = registry.get<Component>(entity);
        derived().on_component_removed(entity, component);
    }

    template <typename Component>
    void on_update(entt::registry& registry, entt::entity entity)
    {
        auto& component = registry.get<Component>(entity);
        derived().on_component_changed(entity, component);
    }

    template <typename Component>
    void register_component_callbacks_single()
    {
        if constexpr (ecs::OnComponentAdded<Derived, Component>)
        {
            registry.on_construct<Component>().template connect<&System::on_construct<Component>>(this);
        }

        if constexpr (ecs::OnComponentRemoved<Derived, Component>)
        {
            registry.on_construct<Component>().template connect<&System::on_destroy<Component>>(this);
        }

        if constexpr (ecs::OnComponentChanged<Derived, Component>)
        {
            registry.on_construct<Component>().template connect<&System::on_update<Component>>(this);
        }
    }

    void register_component_callbacks()
    {
        (register_component_callbacks_single<Components>(), ...);
    }

private:
    StringId name;
};
}
