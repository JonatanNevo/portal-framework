
.. _program_listing_file_portal_engine_renderer_vulkan_vulkan_device.h:

Program Listing for File vulkan_device.h
========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_vulkan_device.h>` (``portal\engine\renderer\vulkan\vulkan_device.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "device/vulkan_physical_device.h"
   #include "portal/engine/renderer/vulkan/allocated_buffer.h"
   #include "portal/engine/renderer/vulkan/allocated_image.h"
   #include "portal/engine/renderer/vulkan/vulkan_common.h"
   #include "portal/core/strings/string_id.h"
   #include "portal/engine/renderer/device/device.h"
   #include "portal/engine/renderer/vulkan/queue/vulkan_queue.h"
   
   namespace portal::renderer::vulkan
   {
   class DescriptorLayoutBuilder;
   }
   
   namespace portal::renderer::vulkan
   {
   struct BufferBuilder;
   class PipelineBuilder;
   class VulkanQueue;
   
   class VulkanDevice final : public Device
   {
   public:
       enum class QueueType
       {
           Graphics,
           Compute,
           Transfer,
           Present
       };
   
   public:
       VulkanDevice(const VulkanPhysicalDevice& physical_device, const VulkanPhysicalDevice::Features& device_features);
   
       // Queue Operations
       void add_present_queue(Surface& surface);
   
       // Object Creation
       [[nodiscard]] AllocatedBuffer create_buffer(const BufferBuilder& builder) const;
       [[nodiscard]] std::shared_ptr<AllocatedBuffer> create_buffer_shared(const BufferBuilder& builder) const;
       [[nodiscard]] ImageAllocation create_image(const ImageBuilder& builder) const;
       [[nodiscard]] vk::raii::ImageView create_image_view(const vk::ImageViewCreateInfo& info) const;
       [[nodiscard]] vk::raii::Sampler create_sampler(const vk::SamplerCreateInfo& info) const;
   
       [[nodiscard]] vk::raii::DescriptorSetLayout create_descriptor_set_layout(portal::renderer::vulkan::DescriptorLayoutBuilder& builder) const;
       [[nodiscard]] vk::raii::PipelineLayout create_pipeline_layout(const vk::PipelineLayoutCreateInfo& pipeline_layout_info) const;
       [[nodiscard]] vk::raii::ShaderModule create_shader_module(const Buffer& code) const;
       [[nodiscard]] vk::raii::Pipeline create_pipeline(vulkan::PipelineBuilder& builder) const;
   
       // Command Submission
   
       void immediate_submit(std::function<void(const vk::raii::CommandBuffer&)>&& function) const;
   
       // Synchronization
   
       void wait_for_fences(vk::ArrayProxy<const vk::Fence> fences, bool wait_all, size_t timeout = std::numeric_limits<size_t>::max()) const;
       void wait_idle() const;
   
       // Getters
   
       [[nodiscard]] vk::raii::Device& get_handle() { return device; }
       [[nodiscard]] const vk::raii::Device& get_handle() const { return device; }
       [[nodiscard]] const VulkanQueue& get_graphics_queue() const;
       [[nodiscard]] const VulkanQueue& get_compute_queue() const;
       [[nodiscard]] const VulkanQueue& get_transfer_queue() const;
       [[nodiscard]] const VulkanQueue& get_present_queue() const;
   
   
       // Debug
   
       template <typename T>
       void set_debug_name(const T& handle, StringId name) const
       {
           set_debug_name(handle, name.string.data());
       }
   
       template <typename T>
       void set_debug_name(const T& handle, const char* name) const
       {
           using HandleType = std::remove_cv_t<std::remove_reference_t<T>>;
   
           uint64_t object_handle;
           if constexpr (requires { typename HandleType::CppType; })
           {
               // RAII wrapper type (like vk::raii::Buffer)
               object_handle = VK_HANDLE_CAST(handle);
           }
           else
           {
               // Plain handle type (like vk::Buffer)
               object_handle = reinterpret_cast<uint64_t>(static_cast<typename HandleType::CType>(handle));
           }
   
           set_debug_name(HandleType::objectType, object_handle, name);
       }
   
       void set_debug_name(vk::ObjectType type, uint64_t handle, const char* name) const;
   
   private:
       struct ImmediateCommandBuffer
       {
           vk::raii::CommandPool command_pool = nullptr;
           vk::raii::CommandBuffer command_buffer = nullptr;
           vk::raii::Fence fence = nullptr;
       };
   
   private:
       void initialize_immediate_commands();
   
   private:
       const VulkanPhysicalDevice& physical_device;
       vk::raii::Device device = nullptr;
   
       std::unordered_map<QueueType, VulkanQueue> queues;
   
       ImmediateCommandBuffer immediate_command_buffer;
       vk::raii::PipelineCache pipeline_cache = nullptr;
   
       bool debug_marker_enabled = false;
   };
   } // portal
