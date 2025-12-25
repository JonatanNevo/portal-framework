
.. _program_listing_file_portal_engine_renderer_descriptor_writer.h:

Program Listing for File descriptor_writer.h
============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptor_writer.h>` (``portal\engine\renderer\descriptor_writer.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <deque>
   #include <vulkan/vulkan_raii.hpp>
   
   
   namespace portal::renderer::vulkan
   {
   class VulkanDevice;
   class AllocatedBuffer;
   }
   
   namespace portal::renderer::vulkan
   {
   struct DescriptorWriter
   {
   public:
       void write_image(
           uint32_t binding,
           const vk::raii::ImageView& image_view,
           const vk::raii::Sampler& sampler,
           vk::ImageLayout layout,
           vk::DescriptorType type
       );
       void write_buffer(uint32_t binding, renderer::vulkan::AllocatedBuffer& buffer, size_t size, size_t offset, vk::DescriptorType type);
   
       void clear();
       void update_set(const renderer::vulkan::VulkanDevice& device, const vk::raii::DescriptorSet& set);
   
       std::deque<vk::DescriptorImageInfo> image_infos;
       std::deque<vk::DescriptorBufferInfo> buffer_infos;
       std::vector<vk::WriteDescriptorSet> writes;
   };
   }
