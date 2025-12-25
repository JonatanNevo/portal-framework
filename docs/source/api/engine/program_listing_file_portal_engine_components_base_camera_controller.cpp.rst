
.. _program_listing_file_portal_engine_components_base_camera_controller.cpp:

Program Listing for File base_camera_controller.cpp
===================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_components_base_camera_controller.cpp>` (``portal\engine\components\base_camera_controller.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "base_camera_controller.h"
   
   namespace portal
   {
   void BaseCameraController::move_up(float scale)
   {
       if (!should_move)
           return;
   
       constexpr glm::vec3 up_direction(0.0f, 1.0f, 0.0f);
   
       moved = true;
       position_delta += up_direction * scale;
   }
   
   void BaseCameraController::move_right(float scale)
   {
       if (!should_move)
           return;
   
       constexpr glm::vec3 up_direction(0.0f, 1.0f, 0.0f);
       const glm::vec3 right_direction = glm::cross(forward_direction, up_direction);
   
       moved = true;
       position_delta += right_direction * scale;
   }
   
   void BaseCameraController::move_forward(float scale)
   {
       if (!should_move)
           return;
   
       moved = true;
       position_delta += forward_direction * scale;
   }
   
   void BaseCameraController::look_to(glm::vec2 screen_space_target)
   {
       if (!should_move)
           return;
   
       moved = true;
   
       if (reset_mouse_on_next_move)
       {
           // Consume the first warp after locking the cursor so we don't get a jump
           last_mouse_position = screen_space_target;
           mouse_delta = glm::vec2{0.f};
           moved = false;
           reset_mouse_on_next_move = false;
           return;
       }
   
       mouse_delta = (screen_space_target - last_mouse_position) * 0.002f;
       last_mouse_position = screen_space_target;
       if (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f)
       {
           moved = true;
       }
   }
   
   void BaseCameraController::mark_as_moving()
   {
       if (should_move == false)
           reset_mouse_on_next_move = true;
   
       should_move = true;
   }
   
   void BaseCameraController::mark_as_stopped_moving()
   {
       should_move = false;
   }
   } // portal
