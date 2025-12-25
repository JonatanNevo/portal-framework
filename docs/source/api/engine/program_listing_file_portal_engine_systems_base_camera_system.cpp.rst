
.. _program_listing_file_portal_engine_systems_base_camera_system.cpp:

Program Listing for File base_camera_system.cpp
===============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_systems_base_camera_system.cpp>` (``portal\engine\systems\base_camera_system.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "base_camera_system.h"
   
   #include "portal/application/frame_context.h"
   
   namespace portal
   {
   void BaseCameraSystem::execute(FrameContext& frame, ecs::Registry& registry)
   {
       for (auto&& [entity_id, controller, camera, transform] : group(registry).each())
       {
           auto entity = registry.entity_from_id(entity_id);
           if (!controller.is_moving())
               continue;
   
           constexpr glm::vec3 up_direction(0.0f, 1.0f, 0.0f);
           const glm::vec3 right_direction = glm::cross(controller.forward_direction, up_direction);
   
           transform.set_translation(transform.get_translation() + (controller.position_delta * controller.speed * frame.delta_time));
   
           const float pitch_delta = controller.mouse_delta.y * controller.rotation_speed;
           const float yaw_delta = controller.mouse_delta.x * controller.rotation_speed;
   
           const glm::quat q = glm::normalize(
               glm::cross(
                   glm::angleAxis(-pitch_delta, right_direction),
                   glm::angleAxis(-yaw_delta, glm::vec3(0.f, 1.0f, 0.0f))
               )
           );
           controller.forward_direction = glm::rotate(q, controller.forward_direction);
   
           // TODO: should this be called later? (after the dirty transform recalculation)
           camera.calculate_view(transform.get_translation(), controller.forward_direction);
   
           // Mark transform as dirty for matrix recalculation
           entity.add_component<TransformDirtyTag>();
   
           controller.position_delta = glm::vec3(0.0f);
           controller.mouse_delta = glm::vec2(0.0f);
           controller.moved = false;
       }
   }
   
   void BaseCameraSystem::on_component_added(Entity, CameraComponent& camera_component)
   {
       camera_component.calculate_projection();
   }
   
   void BaseCameraSystem::on_component_changed(Entity, CameraComponent& camera_component)
   {
       camera_component.calculate_projection();
   }
   } // portal
