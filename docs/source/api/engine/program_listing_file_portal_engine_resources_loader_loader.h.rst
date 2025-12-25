
.. _program_listing_file_portal_engine_resources_loader_loader.h:

Program Listing for File loader.h
=================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_loader_loader.h>` (``portal\engine\resources\loader\loader.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/engine/reference.h"
   
   namespace portal
   {
   class ArchiveObject;
   class Resource;
   struct SourceMetadata;
   class ResourceRegistry;
   }
   
   namespace portal::resources
   {
   class ResourceSource;
   
   class ResourceLoader
   {
   public:
       explicit ResourceLoader(ResourceRegistry& registry) : registry(registry) {}
       virtual ~ResourceLoader() = default;
   
       virtual Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) = 0;
   
   protected:
       ResourceRegistry& registry;
   };
   }
