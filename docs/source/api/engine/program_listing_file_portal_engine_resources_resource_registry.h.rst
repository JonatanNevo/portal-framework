
.. _program_listing_file_portal_engine_resources_resource_registry.h:

Program Listing for File resource_registry.h
============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_resource_registry.h>` (``portal\engine\resources\resource_registry.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <llvm/ADT/DenseMap.h>
   #include <llvm/ADT/DenseSet.h>
   #include <portal/core/jobs/scheduler.h>
   
   #include "portal/engine/resources/utils.h"
   #include "reference_manager.h"
   #include "database/resource_database.h"
   #include "loader/loader_factory.h"
   #include "portal/core/concurrency/spin_lock.h"
   #include "portal/engine/reference.h"
   #include "portal/engine/ecs/registry.h"
   #include "portal/engine/modules/scheduler_module.h"
   #include "portal/engine/modules/system_orchestrator.h"
   #include "portal/engine/resources/resources/resource.h"
   
   namespace portal::renderer::vulkan
   {
   class VulkanTexture;
   }
   
   namespace portal
   {
   template <ResourceConcept T>
   class ResourceReference;
   
   class ResourceRegistry final : public Module<ReferenceManager, ResourceDatabase, SchedulerModule, SystemOrchestrator>
   {
   public:
       ResourceRegistry(ModuleStack& stack, const RendererContext& context);
       ~ResourceRegistry() noexcept override;
   
       template <ResourceConcept T>
       ResourceReference<T> load(StringId resource_id)
       {
           auto type = utils::to_resource_type<T>();
           create_resource(resource_id, type);
   
           auto reference = ResourceReference<T>(resource_id, *this, get_dependency<ReferenceManager>());
           return reference;
       }
   
       template <ResourceConcept T>
       ResourceReference<T> immediate_load(StringId resource_id)
       {
           auto type = utils::to_resource_type<T>();
           create_resource_immediate(resource_id, type);
   
           auto reference = ResourceReference<T>(resource_id, *this, get_dependency<ReferenceManager>());
           return reference;
       }
   
       // TODO: Unload
   
       template <ResourceConcept T>
       ResourceReference<T> get(const StringId resource_id)
       {
           if (resources.contains(resource_id))
               return ResourceReference<T>(resource_id, *this, get_dependency<ReferenceManager>());
   
           auto res = get_dependency<ResourceDatabase>().find(resource_id);
           if (res.has_value())
               return ResourceReference<T>(res->resource_id, *this, get_dependency<ReferenceManager>());
   
           return ResourceReference<T>(INVALID_STRING_ID, *this, get_dependency<ReferenceManager>());
       }
   
       template <ResourceConcept T, typename... Args>
       Reference<T> allocate(const StringId id, Args&&... args)
       {
           // TODO: add some dependency checks?
           auto ref = make_reference<T>(std::forward<Args>(args)...);
   
           std::lock_guard guard(lock);
           resources[id] = ref;
           return ref;
       }
   
       Job<Reference<Resource>> load_direct(const SourceMetadata& meta, const resources::ResourceSource& source);
   
       // TODO: remove from here
       void wait_all(std::span<Job<>> jobs);
   
       void configure_ecs_registry(ecs::Registry& ecs_registry);
   
   protected:
       [[nodiscard]] std::expected<Reference<Resource>, ResourceState> get_resource(const StringId& id);
   
       void create_resource(const StringId& resource_id, ResourceType type);
   
       void create_resource_immediate(const StringId& resource_id, ResourceType type);
   
       Job<Reference<Resource>> load_resource(StringId handle);
   
   private:
       template <ResourceConcept T>
       friend class ResourceReference;
   
       SpinLock lock;
       // Resource container, all resource are managed
       // TODO: use custom allocator to have the resources next to each other on the heap
   #ifdef PORTAL_DEBUG
       std::unordered_map<StringId, Reference<Resource>> resources;
   #else
       llvm::DenseMap<StringId, Reference<Resource>> resources;
   #endif
       llvm::DenseSet<StringId> pending_resources;
       llvm::DenseSet<StringId> errored_resources;
   
       resources::LoaderFactory loader_factory;
   };
   } // portal
