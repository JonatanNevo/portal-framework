
.. _program_listing_file_portal_engine_ecs_registry.h:

Program Listing for File registry.h
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_ecs_registry.h>` (``portal\engine\ecs\registry.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <entt/entity/registry.hpp>
   
   #include "entity.h"
   #include "system_base.h"
   
   namespace portal::ecs
   {
   class Registry
   {
   public:
       Registry();
       ~Registry();
   
       Entity entity_from_id(entt::entity id);
       Entity find_or_create(const StringId& name);
       Entity create_entity(const StringId& name = INVALID_STRING_ID);
       Entity find_or_create_child(Entity parent, const StringId& name);
       Entity create_child_entity(Entity parent, const StringId& name = INVALID_STRING_ID);
   
       [[nodiscard]] Entity get_env_entity() const;
   
       void destroy_entity(Entity entity, bool exclude_children = false);
   
       template <typename C>
       void clear()
       {
           registry.clear<C>();
       }
   
       template <typename... T>
       auto view()
       {
           return view_raw<T...>() | std::views::transform([this](const auto entity) { return Entity{entity, registry}; });
       }
   
       template <typename... T>
       auto group()
       {
           return registry.group<T...>();
       }
   
       template <typename... T, ComponentView... V>
       auto group(V&&... views)
       {
           return registry.group<T...>((views.comp_view)...);
       }
   
       template <typename C, typename... Args>
       void add_default_component(Args&&... args)
       {
           registry.on_construct<entt::entity>().connect<&entt::registry::emplace_or_replace<C>>(std::forward<Args>(args)...);
       }
   
       template <SystemConcept S, typename... Args>
       S register_system(Args&&... args)
       {
           S system(std::forward<Args>(args)...);
           register_system(system);
           return system;
       }
   
       template <SystemConcept S>
       void register_system(S& system)
       {
           system.register_to(registry);
       }
   
       [[nodiscard]] entt::registry& get_raw_registry() { return registry; }
   
   private:
       template <typename... T>
       auto view_raw()
       {
           return registry.view<T...>();
       }
   
   private:
       entt::entity env_entity = entt::null;
       entt::registry registry;
   };
   } // portal
