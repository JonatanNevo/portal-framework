
.. _program_listing_file_portal_engine_renderer_descriptor_layout_builder.h:

Program Listing for File descriptor_layout_builder.h
====================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptor_layout_builder.h>` (``portal\engine\renderer\descriptor_layout_builder.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <vector>
   #include <vulkan/vulkan_raii.hpp>
   
   #include "portal/core/strings/string_id.h"
   
   
   namespace portal::renderer::vulkan
   {
   class DescriptorLayoutBuilder
   {
   public:
       std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
       StringId name = INVALID_STRING_ID;
   
       DescriptorLayoutBuilder& add_binding(size_t binding, vk::DescriptorType type, vk::ShaderStageFlags shader_stages, size_t count = 1);
       DescriptorLayoutBuilder& set_name(const StringId& layout_name);
   
       void clear();
       vk::raii::DescriptorSetLayout build(const vk::raii::Device& device);
   };
   } // portal
