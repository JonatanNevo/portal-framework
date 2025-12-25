
.. _program_listing_file_portal_engine_renderer_vulkan_vulkan_init.h:

Program Listing for File vulkan_init.h
======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_vulkan_init.h>` (``portal\engine\renderer\vulkan\vulkan_init.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <vulkan/vulkan_raii.hpp>
   
   namespace portal::renderer::vulkan
   {
   vk::raii::CommandPool create_command_pool(
       const vk::raii::Device& device,
       uint32_t queue_family_index,
       vk::CommandPoolCreateFlags flags = {}
   );
   
   vk::raii::CommandBuffer allocate_command_buffer(
       const vk::raii::Device& device,
       const vk::raii::CommandPool& command_pool,
       vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
   );
   
   std::vector<vk::raii::CommandBuffer> allocate_command_buffers(
       const vk::raii::Device& device,
       const vk::raii::CommandPool& command_pool,
       uint32_t count,
       vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
   );
   
   vk::raii::ImageView create_image_view(
       const vk::raii::Device& device,
       const vk::Image& image,
       uint32_t mip_level,
       vk::Format format,
       vk::ImageAspectFlags aspect_flags
   );
   }
