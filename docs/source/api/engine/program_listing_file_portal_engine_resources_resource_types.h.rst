
.. _program_listing_file_portal_engine_resources_resource_types.h:

Program Listing for File resource_types.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_resource_types.h>` (``portal\engine\resources\resource_types.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <cstdint>
   #include <string_view>
   #include <portal/core/common.h>
   #include <portal/core/debug/assert.h>
   
   #include <spdlog/spdlog.h>
   
   namespace portal
   {
   enum class ResourceState: uint8_t
   {
       Unknown = 0,
       Loaded  = 1,
       Missing = 2,
       Pending = 3,
       Error   = 4,
       Null
   };
   
   enum class ResourceType: uint16_t
   {
       Unknown   = 0,
       Material  = 1,
       Texture   = 2,
       Shader    = 3,
       Mesh      = 4,
       Scene     = 6,
       Composite = 7,
   };
   
   enum class SourceFormat: uint8_t
   {
       Unknown,
       Memory,            // Source exists in memory
       Image,             // Image formats, e.g. PNG, JPEG
       Texture,           // Ktx or other texture formats
       Material,          // Material files, e.g. MDL
       Obj,               // Wavefront .obj files
       Shader,            // Shader files, e.g. slang
       PrecompiledShader, // Precompiled shader files, e.g. spv
       Glft               // GLTF files
   };
   
   namespace utils
   {
       std::optional<std::pair<ResourceType, SourceFormat>> find_extension_type(std::string_view extension);
   }
   }
   
   template <>
   struct std::hash<portal::ResourceType>
   {
       size_t operator()(const portal::ResourceType& type) const noexcept
       {
           return static_cast<uint16_t>(type);
       }
   };
