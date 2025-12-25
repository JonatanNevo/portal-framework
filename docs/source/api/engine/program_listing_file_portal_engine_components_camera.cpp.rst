
.. _program_listing_file_portal_engine_components_camera.cpp:

Program Listing for File camera.cpp
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_components_camera.cpp>` (``portal\engine\components\camera.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "camera.h"
   
   namespace portal
   {
   void CameraComponent::calculate_projection()
   {
       projection = glm::perspectiveFov(glm::radians(vertical_fov), static_cast<float>(width), static_cast<float>(height), near_clip, far_clip);
       inverse_projection = glm::inverse(projection);
   }
   
   void CameraComponent::calculate_view(glm::vec3 position, glm::vec3 forward_direction)
   {
       view = glm::lookAt(position, position + forward_direction, glm::vec3(0, 1, 0));
       inverse_view = glm::inverse(view);
   }
   
   void CameraComponent::set_viewport_bounds(glm::uvec4 bounds)
   {
       const float new_width = static_cast<float>(bounds.z - bounds.x);
       const float new_height = static_cast<float>(bounds.w - bounds.y);
   
       if (new_width != width || new_height != height)
       {
           width = static_cast<uint32_t>(new_width);
           height = static_cast<uint32_t>(new_height);
   
           calculate_projection();
       }
   }
   } // portal
