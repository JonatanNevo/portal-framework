
.. _program_listing_file_portal_engine_resources_loader_shader_loader.h:

Program Listing for File shader_loader.h
========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_loader_shader_loader.h>` (``portal\engine\resources\loader\shader_loader.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/resources/loader/loader.h"
   
   namespace portal
   {
   class RendererContext;
   }
   
   
   namespace portal::resources
   {
   class ShaderLoader final : public ResourceLoader
   {
   public:
       ShaderLoader(ResourceRegistry& registry, const RendererContext& context);
   
       Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;
   
   protected:
       [[nodiscard]] Reference<Resource> load_shader(const SourceMetadata& meta, const ResourceSource& source) const;
       [[nodiscard]] Reference<Resource> load_precompiled_shader(const SourceMetadata& meta, const ResourceSource& source) const;
   
   private:
       const RendererContext& context;
   };
   } // portal
