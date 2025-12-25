
.. _program_listing_file_portal_engine_renderer_rendering_types.h:

Program Listing for File rendering_types.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_rendering_types.h>` (``portal\engine\renderer\rendering_types.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <portal/core/glm.h>
   
   #include "vulkan/allocated_buffer.h"
   
   namespace portal::renderer::vulkan
   {
   struct Vertex
   {
       glm::vec3 position;
       float uv_x;
       glm::vec3 normal;
       float uv_y;
       glm::vec4 color;
   };
   
   struct GPUMeshBuffers
   {
       Buffer index_buffer;
       Buffer vertex_buffer;
       vk::DeviceAddress vertex_buffer_address{};
   };
   
   struct GPUCameraData
   {
       glm::mat4 view;
       glm::mat4 proj;
       glm::mat4 view_proj;
       glm::mat4 inverse_view;
       glm::mat4 inverse_proj;
       glm::mat4 inverse_view_proj;
   };
   
   struct GPUSceneData
   {
       glm::mat4 view;
       glm::mat4 proj;
       glm::mat4 view_proj;
       glm::vec4 ambient_color;
       glm::vec4 sunlight_direction; // w for sun power
       glm::vec4 sunlight_color;
   };
   
   struct GPUDrawPushConstants
   {
       glm::mat4 world_matrix;
       vk::DeviceAddress vertex_buffer{};
   };
   
   struct Bounds
   {
       glm::vec3 origin{};
       float sphere_radius{};
       glm::vec3 extents{};
   };
   }
