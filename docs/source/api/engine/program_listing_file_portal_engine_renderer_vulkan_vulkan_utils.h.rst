
.. _program_listing_file_portal_engine_renderer_vulkan_vulkan_utils.h:

Program Listing for File vulkan_utils.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_vulkan_utils.h>` (``portal\engine\renderer\vulkan\vulkan_utils.h``)

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
   vk::SampleCountFlagBits get_max_usable_sample_count(vk::raii::PhysicalDevice& physical_device);
   
   void transition_image_layout(
       const vk::raii::CommandBuffer& command_buffer,
       const vk::Image& image,
       uint32_t mip_level,
       vk::ImageLayout old_layout,
       vk::ImageLayout new_layout
   );
   
   void transition_image_layout(
       const vk::raii::CommandBuffer& command_buffer,
       const vk::Image& image,
       uint32_t mip_level,
       vk::ImageLayout old_layout,
       vk::ImageLayout new_layout,
       vk::AccessFlags2 src_access_mask,
       vk::AccessFlags2 dst_access_mask,
       vk::PipelineStageFlags2 src_stage_mask,
       vk::PipelineStageFlags2 dst_stage_mask,
       vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor
   );
   
   void transition_image_layout(
       const vk::raii::CommandBuffer& command_buffer,
       const vk::Image& image,
       const vk::ImageSubresourceRange& subresource,
       vk::ImageLayout old_layout,
       vk::ImageLayout new_layout,
       vk::AccessFlags2 src_access_mask,
       vk::AccessFlags2 dst_access_mask,
       vk::PipelineStageFlags2 src_stage_mask,
       vk::PipelineStageFlags2 dst_stage_mask
   );
   
   void copy_image_to_image(
       const vk::raii::CommandBuffer& command_buffer,
       const vk::Image& source,
       const vk::Image& dest,
       vk::Extent2D src_size,
       vk::Extent2D dst_size
   );
   }
