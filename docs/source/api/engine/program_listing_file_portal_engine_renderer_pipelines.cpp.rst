
.. _program_listing_file_portal_engine_renderer_pipelines.cpp:

Program Listing for File pipelines.cpp
======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_pipelines.cpp>` (``portal\engine\renderer\pipelines.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "pipelines.h"
   
   #include "portal/core/files/file_system.h"
   
   namespace portal::renderer::vulkan
   {
   vk::raii::ShaderModule load_shader_module(const std::filesystem::path& path, const vk::raii::Device& device)
   {
       const auto buffer = FileSystem::read_file_binary(path);
   
       const vk::ShaderModuleCreateInfo shader_module_create_info{
           .codeSize = buffer.size * sizeof(char),
           .pCode = buffer.as<uint32_t*>()
       };
   
       return device.createShaderModule(shader_module_create_info);
   }
   }
