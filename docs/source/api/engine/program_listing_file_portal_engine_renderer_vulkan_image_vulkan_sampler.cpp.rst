
.. _program_listing_file_portal_engine_renderer_vulkan_image_vulkan_sampler.cpp:

Program Listing for File vulkan_sampler.cpp
===========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_vulkan_image_vulkan_sampler.cpp>` (``portal\engine\renderer\vulkan\image\vulkan_sampler.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "vulkan_sampler.h"
   
   #include "portal/engine/renderer/vulkan/vulkan_context.h"
   #include "portal/engine/renderer/vulkan/vulkan_device.h"
   #include "portal/engine/renderer/vulkan/vulkan_enum.h"
   
   namespace portal::renderer::vulkan
   {
   VulkanSampler::VulkanSampler(const StringId& id, const SamplerProperties& properties, const VulkanDevice& device) : id(id), properties(properties)
   {
       const vk::SamplerCreateInfo sampler_info{
           .magFilter = to_filter(properties.filter),
           .minFilter = to_filter(properties.filter),
           .mipmapMode = to_mipmap_mode(properties.mipmap_mode),
           .addressModeU = to_address_mode(properties.wrap),
           .addressModeV = to_address_mode(properties.wrap),
           .addressModeW = to_address_mode(properties.wrap),
           .anisotropyEnable = false,
           .maxAnisotropy = 1.f,
           .minLod = properties.min_lod,
           .maxLod = properties.max_lod,
           .borderColor = vk::BorderColor::eFloatOpaqueWhite,
       };
   
       sampler = device.create_sampler(sampler_info);
       device.set_debug_name(sampler, id);
   }
   
   vk::Sampler VulkanSampler::get_vk_sampler() const
   {
       return sampler;
   }
   
   const SamplerProperties& VulkanSampler::get_prop() const
   {
       return properties;
   }
   } // portal
