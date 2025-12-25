
.. _program_listing_file_portal_engine_scene_scene.h:

Program Listing for File scene.h
================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_scene_scene.h>` (``portal\engine\scene\scene.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/ecs/entity.h"
   #include "portal/engine/ecs/registry.h"
   #include "portal/engine/resources/resources/resource.h"
   
   namespace portal
   {
   class Scene final : public Resource
   {
   public:
       explicit Scene(const StringId& name);
   
       [[nodiscard]] ecs::Registry& get_registry() const { return const_cast<ecs::Registry&>(registry); }
   
       Entity get_scene_entity() const { return scene_entity; }
   
   private:
       ecs::Registry registry;
       Entity scene_entity;
   };
   } // portal
