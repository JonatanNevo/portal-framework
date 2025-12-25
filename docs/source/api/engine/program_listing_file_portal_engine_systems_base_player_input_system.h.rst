
.. _program_listing_file_portal_engine_systems_base_player_input_system.h:

Program Listing for File base_player_input_system.h
===================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_systems_base_player_input_system.h>` (``portal\engine\systems\base_player_input_system.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/components/base.h"
   #include "portal/engine/components/base_camera_controller.h"
   #include "portal/engine/ecs/system.h"
   
   namespace portal
   {
   class BasePlayerInputSystem : public ecs::System<
           BasePlayerInputSystem,
           ecs::Owns<InputComponent>,
           ecs::Views<BaseCameraController>,
           ecs::Views<PlayerTag>
       >
   {
   public:
       static void execute(ecs::Registry& registry);
   
       static void enable_mouse(const InputManager* input);
       static void disable_mouse(const InputManager* input);
   
       [[nodiscard]] static StringId get_name() { return STRING_ID("Base Player Input"); };
   };
   } // portal
