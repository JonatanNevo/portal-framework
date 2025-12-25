
.. _program_listing_file_portal_engine_resources_source_resource_source.h:

Program Listing for File resource_source.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_source_resource_source.h>` (``portal\engine\resources\source\resource_source.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/core/buffer.h"
   
   namespace portal::resources
   {
   class ResourceSource
   {
   public:
       virtual ~ResourceSource() = default;
       [[nodiscard]] virtual Buffer load() const = 0;
       [[nodiscard]] virtual Buffer load(size_t offset, size_t size) const = 0;
       [[nodiscard]] virtual std::unique_ptr<std::istream> stream() const = 0;
   };
   }
