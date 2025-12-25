
.. _program_listing_file_portal_engine_renderer_vulkan_queue_vulkan_queue.h:

Program Listing for File vulkan_queue.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_queue_vulkan_queue.h>` (``portal\engine\renderer\vulkan\queue\vulkan_queue.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <vulkan/vulkan_raii.hpp>
   #include "portal/engine/renderer/queue/queue.h"
   
   namespace portal::renderer::vulkan
   {
   class VulkanDevice;
   
   class VulkanQueue final : public Queue
   {
   public:
       VulkanQueue(const VulkanDevice& device, size_t family_index, const vk::QueueFamilyProperties& properties, size_t index, bool presentable);
   
       // TODO: use generic type
       void submit(const vk::SubmitInfo2& info, vk::Fence fence) const;
       vk::Result present(const vk::PresentInfoKHR& info) const;
   
       [[nodiscard]] size_t get_index() const override;
       [[nodiscard]] size_t get_family_index() const;
       [[nodiscard]] bool is_presentable() const;
       [[nodiscard]] vk::QueueFamilyProperties get_properties() const;
       [[nodiscard]] vk::Queue get_handle() const;
   
   private:
       vk::raii::Queue queue;
       size_t family_index;
       size_t index;
   
       vk::QueueFamilyProperties properties;
       bool presentable;
   };
   } // portal
