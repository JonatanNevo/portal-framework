
.. _program_listing_file_portal_engine_renderer_vulkan_surface_vulkan_surface.h:

Program Listing for File vulkan_surface.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_surface_vulkan_surface.h>` (``portal\engine\renderer\vulkan\surface\vulkan_surface.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/engine/renderer/surface/surface.h"
   #include "portal/engine/renderer/vulkan/vulkan_context.h"
   
   namespace portal::renderer::vulkan
   {
   class VulkanSurface final : public Surface
   {
   public:
       explicit VulkanSurface(const VulkanContext& context, const SurfaceProperties& properties);
   
       [[nodiscard]] const SurfaceCapabilities& get_capabilities() const override;
       [[nodiscard]] glm::ivec2 get_extent() const override;
   
       [[nodiscard]] vk::SurfaceKHR get_vulkan_surface() const;
   
       [[nodiscard]] SurfaceType get_type() const override;
       [[nodiscard]] size_t get_min_frames_in_flight() const override;
   
   private:
       vk::raii::SurfaceKHR surface = nullptr;
       SurfaceCapabilities capabilities;
   };
   }
