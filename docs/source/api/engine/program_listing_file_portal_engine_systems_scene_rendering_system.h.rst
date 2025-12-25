
.. _program_listing_file_portal_engine_systems_scene_rendering_system.h:

Program Listing for File scene_rendering_system.h
=================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_systems_scene_rendering_system.h>` (``portal\engine\systems\scene_rendering_system.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/ecs/system.h"
   #include "portal/engine/components/mesh.h"
   #include "portal/engine/components/transform.h"
   
   namespace portal
   {
   class SceneRenderingSystem : public ecs::System<SceneRenderingSystem, ecs::Owns<StaticMeshComponent>, ecs::Views<TransformComponent>>
   {
   public:
       static void execute(FrameContext& frame, ecs::Registry& registry);
   
       static void update_global_descriptors(FrameContext& frame, ecs::Registry& registry);
       static void add_static_mesh_to_context(FrameContext& frame, ecs::Registry& registry);
   
       [[nodiscard]] static StringId get_name() { return STRING_ID("Scene Rendering"); };
   };
   } // portal
