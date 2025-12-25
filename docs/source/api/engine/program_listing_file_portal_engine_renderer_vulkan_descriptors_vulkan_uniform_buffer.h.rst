
.. _program_listing_file_portal_engine_renderer_vulkan_descriptors_vulkan_uniform_buffer.h:

Program Listing for File vulkan_uniform_buffer.h
================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_descriptors_vulkan_uniform_buffer.h>` (``portal\engine\renderer\vulkan\descriptors\vulkan_uniform_buffer.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/engine/reference.h"
   #include "portal/engine/renderer/descriptors/uniform_buffer.h"
   #include "portal/engine/renderer/vulkan/vulkan_device.h"
   
   namespace portal::renderer::vulkan
   {
   class VulkanUniformBuffer final : public UniformBuffer
   {
   public:
       VulkanUniformBuffer(size_t size, const VulkanDevice& device);
       ~VulkanUniformBuffer() override;
   
       void set_data(Buffer data, size_t offset) override;
       const Buffer& get_data() const override;
   
       const vk::DescriptorBufferInfo& get_descriptor_buffer_info() const;
   
   private:
       void release();
       void init();
   
   private:
       AllocatedBuffer buffer;
       size_t size;
   
       Buffer local_storage = nullptr;
       vk::DescriptorBufferInfo descriptor_buffer_info;
   
       const VulkanDevice& device;
   };
   
   class VulkanUniformBufferSet final : public UniformBufferSet
   {
   public:
       VulkanUniformBufferSet(size_t buffer_size, size_t size, const VulkanDevice& device);
   
       Reference<UniformBuffer> get(size_t index) override;
       void set(const Reference<UniformBuffer>& buffer, size_t index) override;
   
       void set_data(Buffer data, size_t offset) override;
       const Buffer& get_data() const override;
   
   private:
       std::unordered_map<size_t, Reference<VulkanUniformBuffer>> buffers;
   
       [[maybe_unused]] const VulkanDevice& device;
   };
   } // portal
