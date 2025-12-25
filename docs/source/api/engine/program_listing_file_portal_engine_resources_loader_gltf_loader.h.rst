
.. _program_listing_file_portal_engine_resources_loader_gltf_loader.h:

Program Listing for File gltf_loader.h
======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_loader_gltf_loader.h>` (``portal\engine\resources\loader\gltf_loader.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <filesystem>
   
   #include "portal/core/jobs/job.h"
   #include "portal/engine/resources/loader/loader.h"
   #include "portal/engine/scene/scene.h"
   
   namespace fastgltf
   {
   struct Mesh;
   struct Material;
   struct Texture;
   class Asset;
   class GltfDataGetter;
   }
   
   namespace portal
   {
   class RendererContext;
   }
   
   namespace portal::renderer
   {
   namespace vulkan
   {
       class VulkanPipeline;
   }
   
   class ShaderVariant;
   class Pipeline;
   class Shader;
   }
   
   
   namespace portal::resources
   {
   class LoaderFactory;
   
   class GltfLoader final : public ResourceLoader
   {
   public:
       GltfLoader(ResourceRegistry& registry, const RendererContext& context);
   
       Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;
       static void enrich_metadata(SourceMetadata& meta, const ResourceSource& source);
   
   protected:
       static fastgltf::Asset load_asset(const SourceMetadata& meta, fastgltf::GltfDataGetter& data);
   
       static std::pair<SourceMetadata, std::unique_ptr<ResourceSource>> find_image_source(
           const std::filesystem::path& base_name,
           const std::filesystem::path& base_path,
           const fastgltf::Asset& asset,
           const fastgltf::Texture& texture
       );
   
       Job<> load_texture(SourceMetadata texture_meta, const fastgltf::Asset& asset, const fastgltf::Texture& texture) const;
       Job<> load_material(SourceMetadata material_meta, const fastgltf::Asset& asset, const fastgltf::Material& material) const;
       Job<> load_mesh(SourceMetadata mesh_meta, const fastgltf::Asset& asset, const fastgltf::Mesh& mesh) const;
       void load_scenes(SourceMetadata meta, const fastgltf::Asset& asset) const;
   
   protected:
       const RendererContext& context;
   
       Reference<renderer::vulkan::VulkanPipeline> g_transparent_pipeline;
       Reference<renderer::vulkan::VulkanPipeline> g_color_pipeline;
   };
   } // portal
