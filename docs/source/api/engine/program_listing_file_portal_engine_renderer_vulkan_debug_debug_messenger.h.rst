
.. _program_listing_file_portal_engine_renderer_vulkan_debug_debug_messenger.h:

Program Listing for File debug_messenger.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_debug_debug_messenger.h>` (``portal\engine\renderer\vulkan\debug\debug_messenger.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <vulkan/vulkan.hpp>
   #include <portal/core/log.h>
   
   namespace portal::renderer::vulkan
   {
   class DebugMessenger
   {
   public:
       DebugMessenger();
   
       int get_error_and_warning_count() const;
       int get_error_count() const;
       int get_warning_count() const;
       int get_info_count() const;
   
       static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
           vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
           vk::DebugUtilsMessageTypeFlagsEXT message_type,
           const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
           void* data
       );
   
   protected:
       int error_count;
       int warning_count;
       int info_count;
   
       vk::Bool32 log(
           vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
           vk::DebugUtilsMessageTypeFlagsEXT message_type,
           const vk::DebugUtilsMessengerCallbackDataEXT* callback_data
       );
   
       spdlog::level::level_enum get_severity(const vk::DebugUtilsMessageSeverityFlagBitsEXT severity);
   };
   } // portal
