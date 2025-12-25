
.. _program_listing_file_portal_engine_components_mesh.h:

Program Listing for File mesh.h
===============================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_components_mesh.h>` (``portal\engine\components\mesh.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/resources/resource_reference.h"
   #include "portal/engine/resources/resources/mesh_geometry.h"
   
   namespace portal
   {
   struct StaticMeshComponent
   {
       ResourceReference<MeshGeometry> mesh;
       std::vector<ResourceReference<renderer::Material>> materials;
       bool visible = true;
   };
   } // portal
