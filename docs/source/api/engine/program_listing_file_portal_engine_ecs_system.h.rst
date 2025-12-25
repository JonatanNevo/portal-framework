
.. _program_listing_file_portal_engine_ecs_system.h:

Program Listing for File system.h
=================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_ecs_system.h>` (``portal\engine\ecs\system.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

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
   template <typename Derived, ComponentOwnership... Components>
   class System : public SystemBase
   {
   public:
       explicit System(
           const ExecutionPolicy policy = ExecutionPolicy::Sequential
       ) : SystemBase(policy)
       {
           static_assert(
               ecs::SystemConcept<Derived>,
               "System<Derived,...>: Derived must implement get_name() and execute() or execute_single(entity)."
           );
       }
   
       void register_to(Registry& registry)
       {
           register_component_callbacks(registry);
           // We call it once for underlying storage optimization
           group(registry);
       }
   
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
                       else
                           scheduler.dispatch_job<void>(
                               derived().execute(registry, scheduler),
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
