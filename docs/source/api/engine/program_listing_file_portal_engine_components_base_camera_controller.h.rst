
.. _program_listing_file_portal_engine_components_base_camera_controller.h:

Program Listing for File base_camera_controller.h
=================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_components_base_camera_controller.h>` (``portal\engine\components\base_camera_controller.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <portal/core/glm.h>
   
   namespace portal
   {
   class BaseCameraController
   {
   public:
       void move_up(float scale);
       void move_right(float scale);
       void move_forward(float scale);
   
       void look_to(glm::vec2 screen_space_target);
   
       void mark_as_moving();
       void mark_as_stopped_moving();
   
       [[nodiscard]] bool is_moving() const { return should_move && moved; }
   
       glm::vec3 position_delta = glm::vec3{0.f};
       glm::vec3 forward_direction = glm::vec3{0.54, -0.42, -0.72};
   
       glm::vec2 mouse_delta{0.f, 0.f};
   
       float speed = 5.f;
       float rotation_speed = 0.3f;
   
       bool moved = false;
   
   private:
       glm::vec2 last_mouse_position{0.f, 0.f};
   
       bool should_move = false;
       bool reset_mouse_on_next_move = false;
   };
   } // portal
