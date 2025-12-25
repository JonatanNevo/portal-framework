
.. _program_listing_file_portal_engine_ecs_system_base.h:

Program Listing for File system_base.h
======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_ecs_system_base.h>` (``portal\engine\ecs\system_base.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

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
   concept HasExecuteJob = requires(System& system, Registry& registry, jobs::Scheduler& scheduler) {
       { system.execute(registry, scheduler) } -> std::same_as<Job<>>;
   };
   
   template <typename System>
   concept HasExecuteJobWithContext = requires(System& system, FrameContext& context, Registry& registry, jobs::Scheduler& scheduler) {
       { system.execute(context, registry, scheduler) } -> std::same_as<Job<>>;
   };
   
   template <typename System>
   concept ParallelExecution = HasExecuteJob<System> || HasExecuteJobWithContext<System>;
   
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
