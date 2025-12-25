
.. _program_listing_file_portal_engine_resources_loader_loader_factory.h:

Program Listing for File loader_factory.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_loader_loader_factory.h>` (``portal\engine\resources\loader\loader_factory.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <memory>
   
   #include "llvm/ADT/DenseMap.h"
   #include "portal/engine/resources/resource_types.h"
   #include "portal/engine/resources/database/resource_database.h"
   #include "loader.h"
   
   namespace portal
   {
   class RendererContext;
   class ResourceRegistry;
   }
   
   namespace portal::renderer::vulkan
   {
   class VulkanContext;
   }
   
   namespace portal::resources
   {
   class StubLoader final : public ResourceLoader
   {
   public:
       explicit StubLoader(ResourceRegistry& registry) : ResourceLoader(registry) {}
   
       Reference<Resource> load(const SourceMetadata&, const ResourceSource&) override { return nullptr; }
   };
   
   
   class LoaderFactory
   {
   public:
       LoaderFactory(ResourceRegistry& registry, const RendererContext& context);
   
       ResourceLoader& get(const SourceMetadata& meta);
   
       static void enrich_metadata(SourceMetadata& meta, const ResourceSource& source);
   
   protected:
       StubLoader stub_loader;
       llvm::DenseMap<ResourceType, std::shared_ptr<ResourceLoader>> loaders;
       const RendererContext& context;
   };
   } // portal
