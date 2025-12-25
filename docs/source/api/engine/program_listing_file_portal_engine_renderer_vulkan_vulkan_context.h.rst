
.. _program_listing_file_portal_engine_renderer_vulkan_vulkan_context.h:

Program Listing for File vulkan_context.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_vulkan_context.h>` (``portal\engine\renderer\vulkan\vulkan_context.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <vulkan/vulkan_raii.hpp>
   
   #include "portal/engine/renderer/vulkan/vulkan_device.h"
   #include "portal/engine/renderer/vulkan/vulkan_instance.h"
   #include "device/vulkan_physical_device.h"
   
   namespace portal::renderer::vulkan
   {
   class VulkanContext final
   {
   public:
       VulkanContext();
       ~VulkanContext();
   
       const vk::raii::Instance& get_instance() const;
       const VulkanDevice& get_device() const;
       VulkanDevice& get_device();
       const VulkanPhysicalDevice& get_physical_device() const;
   
   private:
       vk::raii::Context context{};
       VulkanInstance instance;
   
       VulkanPhysicalDevice& physical_device;
       VulkanDevice device;
   };
   } // portal
