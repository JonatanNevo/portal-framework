
.. _program_listing_file_portal_engine_resources_database_resource_database_factory.h:

Program Listing for File resource_database_factory.h
====================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_database_resource_database_factory.h>` (``portal\engine\resources\database\resource_database_factory.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <filesystem>
   #include <optional>
   
   #include "portal/core/strings/string_id.h"
   
   namespace portal
   {
   enum class DatabaseType
   {
       Unknown,
       Folder
   };
   
   class ModuleStack;
   class ResourceDatabase;
   class ArchiveObject;
   
   struct DatabaseDescription
   {
       DatabaseType type = DatabaseType::Unknown;
       std::optional<std::filesystem::path> path;
   
       void archive(ArchiveObject& archive) const;
       static DatabaseDescription dearchive(ArchiveObject& archive);
   };
   
   
   
   class ResourceDatabaseFactory
   {
   public:
       static std::unique_ptr<ResourceDatabase> create(ModuleStack& stack, const DatabaseDescription& description);
   };
   } // portal
