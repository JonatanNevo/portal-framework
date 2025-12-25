
.. _program_listing_file_portal_engine_resources_database_resource_database_facade.h:

Program Listing for File resource_database_facade.h
===================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_database_resource_database_facade.h>` (``portal\engine\resources\database\resource_database_facade.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <stack>
   
   #include "resource_database.h"
   #include "resource_database_factory.h"
   
   namespace portal
   {
   
   class ResourceDatabaseFacade final : public ResourceDatabase
   {
   public:
       explicit ResourceDatabaseFacade(ModuleStack& stack);
   
       void register_database(const DatabaseDescription& description);
   
       std::expected<SourceMetadata, DatabaseError> find(StringId resource_id) override;
       std::unique_ptr<resources::ResourceSource> create_source(StringId resource_id,SourceMetadata meta) override;
   
       DatabaseError add(StringId resource_id, SourceMetadata meta) override;
       DatabaseError remove(StringId resource_id) override;
   private:
       ModuleStack& stack;
       std::unordered_map<StringId, std::unique_ptr<ResourceDatabase>> databases;
   };
   
   } // portal
