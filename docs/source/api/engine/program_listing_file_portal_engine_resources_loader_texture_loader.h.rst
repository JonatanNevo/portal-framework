
.. _program_listing_file_portal_engine_resources_loader_texture_loader.h:

Program Listing for File texture_loader.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_loader_texture_loader.h>` (``portal\engine\resources\loader\texture_loader.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <vulkan/vulkan_raii.hpp>
   
   #include "portal/core/strings/string_id.h"
   #include "loader.h"
   
   
   namespace portal
   {
   class RendererContext;
   }
   
   namespace portal::resources
   {
   class TextureLoader final : public ResourceLoader
   {
   public:
       TextureLoader(ResourceRegistry& registry, const RendererContext& context);
   
       Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;
       static void enrich_metadata(SourceMetadata& meta, const ResourceSource& source);
   
   protected:
       void create_standalone_texture(const StringId& id, std::span<uint32_t> data, vk::Extent3D extent) const;
   
   private:
       const RendererContext& context;
   };
   } // portal
