
.. _program_listing_file_portal_engine_renderer_vulkan_vulkan_instance.h:

Program Listing for File vulkan_instance.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_vulkan_instance.h>` (``portal\engine\renderer\vulkan\vulkan_instance.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <vulkan/vulkan_raii.hpp>
   
   #include "portal/engine/renderer/device/physical_device.h"
   #include "portal/engine/renderer/vulkan/debug/debug_messenger.h"
   
   namespace portal::renderer::vulkan
   {
   class VulkanPhysicalDevice;
   }
   
   namespace portal::renderer::vulkan
   {
   constexpr std::array REQUIRED_DEVICE_EXTENSIONS = {
       vk::KHRSwapchainExtensionName,
       vk::KHRCalibratedTimestampsExtensionName,
   #if defined(PORTAL_PLATFORM_MACOS)
       vk::KHRPortabilitySubsetExtensionName
   #endif
   };
   
   class VulkanInstance
   {
   public:
       explicit VulkanInstance(vk::raii::Context& context);
   
       [[nodiscard]] const vk::raii::Instance& get_instance() const;
       [[nodiscard]] const DebugMessenger& get_debug_messenger() const;
   
       VulkanPhysicalDevice& get_suitable_gpu() const;
   
   private:
       void query_physical_devices();
       std::vector<const char*> get_required_instance_extensions(bool enable_validation_layers);
   
   private:
       vk::raii::Context& context;
       vk::raii::Instance instance = nullptr;
       vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
       DebugMessenger messenger;
   
       std::vector<std::unique_ptr<VulkanPhysicalDevice>> physical_devices;
   };
   } // portal
