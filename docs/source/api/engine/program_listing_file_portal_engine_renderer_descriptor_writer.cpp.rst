
.. _program_listing_file_portal_engine_renderer_descriptor_writer.cpp:

Program Listing for File descriptor_writer.cpp
==============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptor_writer.cpp>` (``portal\engine\renderer\descriptor_writer.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "descriptor_writer.h"
   
   #include "portal/engine/renderer/vulkan/vulkan_device.h"
   #include "vulkan/allocated_buffer.h"
   
   namespace portal::renderer::vulkan
   {
   void DescriptorWriter::write_image(
       const uint32_t binding,
       const vk::raii::ImageView& image_view,
       const vk::raii::Sampler& sampler,
       const vk::ImageLayout layout,
       const vk::DescriptorType type
   )
   {
       const auto& info = image_infos.emplace_back(
           vk::DescriptorImageInfo{
               .sampler = sampler,
               .imageView = image_view,
               .imageLayout = layout
           }
       );
   
       const vk::WriteDescriptorSet write{
           //left empty for now until we need to write it
           .dstSet = nullptr,
           .dstBinding = binding,
           .descriptorCount = 1,
           .descriptorType = type,
           .pImageInfo = &info
       };
       writes.push_back(write);
   }
   
   void DescriptorWriter::write_buffer(
       const uint32_t binding,
       renderer::vulkan::AllocatedBuffer& buffer,
       const size_t size,
       const size_t offset,
       const vk::DescriptorType type
   )
   {
       const auto& info = buffer_infos.emplace_back(
           vk::DescriptorBufferInfo{
               .buffer = buffer.get_handle(),
               .offset = offset,
               .range = size
           }
       );
   
       const vk::WriteDescriptorSet write{
           //left empty for now until we need to write it
           .dstSet = nullptr,
           .dstBinding = binding,
           .descriptorCount = 1,
           .descriptorType = type,
           .pBufferInfo = &info
       };
       writes.push_back(write);
   }
   
   void DescriptorWriter::clear()
   {
       image_infos.clear();
       buffer_infos.clear();
       writes.clear();
   }
   
   void DescriptorWriter::update_set(const renderer::vulkan::VulkanDevice& device, const vk::raii::DescriptorSet& set)
   {
       for (auto& write : writes)
       {
           write.dstSet = set;
       }
   
       device.get_handle().updateDescriptorSets(writes, {});
   }
   }
