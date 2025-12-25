
.. _program_listing_file_portal_engine_renderer_pipelines.h:

Program Listing for File pipelines.h
====================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_pipelines.h>` (``portal\engine\renderer\pipelines.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <expected>
   #include <filesystem>
   #include "vulkan/vulkan_init.h"
   
   namespace portal::renderer::vulkan
   {
   vk::raii::ShaderModule load_shader_module(const std::filesystem::path& path, const vk::raii::Device& device);
   }
