
.. _program_listing_file_portal_engine_renderer_rendering_context.h:

Program Listing for File rendering_context.h
============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_rendering_context.h>` (``portal\engine\renderer\rendering_context.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <portal/engine/renderer/vulkan/allocated_buffer.h>
   
   #include "portal/engine/renderer/deletion_queue.h"
   #include "portal/engine/renderer/descriptor_allocator.h"
   #include "portal/engine/renderer/rendering_types.h"
   #include "portal/engine/resources/resources/mesh_geometry.h"
   
   namespace portal::renderer {
   }
   
   namespace portal::renderer
   {
   class Material;
   
   struct RenderObject
   {
       uint32_t index_count = 0;
       uint32_t first_index = 0;
       std::shared_ptr<vulkan::AllocatedBuffer> index_buffer = nullptr;
   
       Reference<Material> material = nullptr;
       resources::Bounds bounds{};
   
       glm::mat4 transform = glm::mat4(1.0f);
       vk::DeviceAddress vertex_buffer_address = 0;
   
       [[nodiscard]] bool is_visible(const glm::mat4& view_projection) const
       {
           constexpr std::array clip_space_corners{
               glm::vec3{1, 1, 1},
               glm::vec3{1, 1, -1},
               glm::vec3{1, -1, 1},
               glm::vec3{1, -1, -1},
               glm::vec3{-1, 1, 1},
               glm::vec3{-1, 1, -1},
               glm::vec3{-1, -1, 1},
               glm::vec3{-1, -1, -1},
           };
   
           glm::mat4 matrix = view_projection * transform;
   
           glm::vec3 min = {1.5, 1.5, 1.5};
           glm::vec3 max = {-1.5, -1.5, -1.5};
   
           for (int c = 0; c < 8; c++)
           {
               // project each corner into clip space
               glm::vec4 v = matrix * glm::vec4(bounds.origin + (clip_space_corners[c] * bounds.extents), 1.f);
   
               // perspective correction
               v.x = v.x / v.w;
               v.y = v.y / v.w;
               v.z = v.z / v.w;
   
               min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
               max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
           }
   
           // check the clip space box is within the view
           if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
           {
               return false;
           }
           return true;
       }
   };
   
   // Per frame resource, owned by the renderer
   struct FrameResources
   {
       vk::raii::CommandPool command_pool = nullptr;
       vk::raii::CommandBuffer command_buffer = nullptr;
   
       // Semaphores to signal that images are available for rendering and that rendering has finished
       vk::raii::Semaphore image_available_semaphore = nullptr;
       vk::raii::Semaphore render_finished_semaphore = nullptr;
       // Fence to signal that command buffers are ready to be reused
       vk::raii::Fence wait_fence = nullptr;
   
       DeletionQueue deletion_queue = {};
   
       vk::raii::DescriptorSet global_descriptor_set = nullptr;
       vulkan::AllocatedBuffer scene_data_buffer = nullptr;
       vulkan::DescriptorAllocator frame_descriptors;
   
       FrameResources(auto&& command_pool, auto&& command_buffer, auto&& image_sema, auto&& render_sema, auto&& wait_fence, auto&& descriptors) :
           command_pool(std::move(command_pool)),
           command_buffer(std::move(command_buffer)),
           image_available_semaphore(std::move(image_sema)),
           render_finished_semaphore(std::move(render_sema)),
           wait_fence(std::move(wait_fence)),
           frame_descriptors(std::move(descriptors))
       {}
   
       // Delete copy operations
       FrameResources(const FrameResources&) = delete;
       FrameResources& operator=(const FrameResources&) = delete;
   
       // Default move operations
       FrameResources(FrameResources&&) = default;
       FrameResources& operator=(FrameResources&&) = default;
   
       ~FrameResources()
       {
           deletion_queue.flush();
   
           global_descriptor_set = nullptr;
           frame_descriptors.clear_pools();
           frame_descriptors.destroy_pools();
           scene_data_buffer = nullptr;
       }
   };
   
   
   struct FrameRenderingContext
   {
       // TODO: make this more generic? maybe based on active scene?
       vulkan::GPUSceneData scene_data{};
       vulkan::GPUCameraData camera_data{};
       glm::uvec4 viewport_bounds;
   
       // TODO: switch with generic `Image` class
       vk::Image draw_image;
       vk::ImageView draw_image_view;
       vk::Image depth_image;
       vk::ImageView depth_image_view;
   
       vk::raii::CommandBuffer& command_buffer;
       FrameResources& resources;
   
       llvm::SmallVector<RenderObject> render_objects;
   };
   } // portal
