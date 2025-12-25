
.. _program_listing_file_portal_engine_renderer_renderer_resource.h:

Program Listing for File renderer_resource.h
============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_renderer_resource.h>` (``portal\engine\renderer\renderer_resource.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/engine/resources/resources/resource.h"
   
   namespace portal::renderer
   {
   class RendererResource : public Resource
   {
   public:
       explicit RendererResource(const StringId& id) : Resource(id) {}
   };
   }
