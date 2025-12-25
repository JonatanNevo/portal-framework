
.. _program_listing_file_portal_engine_scene_scene.cpp:

Program Listing for File scene.cpp
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_scene_scene.cpp>` (``portal\engine\scene\scene.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "scene.h"
   
   #include "portal/engine/components/base.h"
   #include "portal/engine/components/relationship.h"
   #include "portal/engine/renderer/rendering_context.h"
   
   namespace portal
   {
   Scene::Scene(const StringId& name) : Resource(name)
   {
       scene_entity = registry.create_entity(name);
       scene_entity.add_component<SceneTag>();
   }
   } // portal
