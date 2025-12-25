
.. _program_listing_file_portal_engine_renderer_vulkan_allocated_buffer.h:

Program Listing for File allocated_buffer.h
===========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_allocated_buffer.h>` (``portal\engine\renderer\vulkan\allocated_buffer.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/renderer/vulkan/base/allocated.h"
   #include "portal/engine/renderer/vulkan/base/builder_base.h"
   
   namespace portal::renderer::vulkan
   {
   class AllocatedBuffer;
   class VulkanDevice;
   
   struct BufferBuilder final : public BuilderBase<BufferBuilder, vk::BufferCreateInfo>
   {
   public:
       explicit BufferBuilder(vk::DeviceSize size);
   
       AllocatedBuffer build(const VulkanDevice& device) const;
       std::shared_ptr<AllocatedBuffer> build_shared(const VulkanDevice& device) const;
       BufferBuilder& with_flags(vk::BufferCreateFlags flags);
       BufferBuilder& with_usage(vk::BufferUsageFlags usage);
   
   private:
       using ParentType = BuilderBase;
   };
   
   class AllocatedBuffer final : public allocation::Allocated<vk::Buffer>
   {
   public:
       static AllocatedBuffer create_staging_buffer(const VulkanDevice& device, vk::DeviceSize size, const void* data);
   
       template <typename T>
       static AllocatedBuffer create_staging_buffer(const VulkanDevice& device, const std::span<T>& data);
   
       template <typename T>
       static AllocatedBuffer create_staging_buffer(const VulkanDevice& device, const T& data);
   
       AllocatedBuffer();
       AllocatedBuffer(std::nullptr_t) : AllocatedBuffer() {}
   
       AllocatedBuffer(AllocatedBuffer&& other) noexcept;
       AllocatedBuffer& operator=(AllocatedBuffer&& other) noexcept;
       AllocatedBuffer& operator=(std::nullptr_t) noexcept override;
   
   
       AllocatedBuffer(const AllocatedBuffer&) = delete;
       AllocatedBuffer& operator=(const AllocatedBuffer&) = delete;
   
   
       ~AllocatedBuffer() override;
   
       [[nodiscard]] uint64_t get_device_address() const;
   
       [[nodiscard]] vk::DeviceSize get_size() const;
   
   protected:
       AllocatedBuffer(const VulkanDevice& device, const BufferBuilder& builder);
       friend struct BufferBuilder;
   
   private:
       vk::DeviceSize size = 0;
   };
   
   template <typename T>
   AllocatedBuffer AllocatedBuffer::create_staging_buffer(const VulkanDevice& device, const std::span<T>& data)
   {
       return create_staging_buffer(device, data.size() * sizeof(T), data.data());
   }
   
   template <typename T>
   AllocatedBuffer AllocatedBuffer::create_staging_buffer(const VulkanDevice& device, const T& data)
   {
       return create_staging_buffer(device, sizeof(T), &data);
   }
   } // portal
