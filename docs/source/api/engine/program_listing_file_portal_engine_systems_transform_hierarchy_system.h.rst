
.. _program_listing_file_portal_engine_systems_transform_hierarchy_system.h:

Program Listing for File transform_hierarchy_system.h
=====================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_systems_transform_hierarchy_system.h>` (``portal\engine\systems\transform_hierarchy_system.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/ecs/system.h"
   
   #include "portal/engine/components/transform.h"
   
   namespace portal
   {
   class TransformHierarchySystem final : public ecs::System<TransformHierarchySystem, ecs::Owns<TransformDirtyTag>, ecs::Owns<TransformComponent>>
   {
   public:
       static void execute(ecs::Registry& registry);
   
       static void on_component_added(Entity entity, TransformComponent& transform);
       static void on_component_changed(Entity entity, TransformComponent& transform);
   
       [[nodiscard]] static StringId get_name() { return STRING_ID("Transform Hierarchy"); };
   };
   } // portal
