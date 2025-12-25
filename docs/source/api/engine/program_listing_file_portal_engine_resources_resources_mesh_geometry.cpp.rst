
.. _program_listing_file_portal_engine_resources_resources_mesh_geometry.cpp:

Program Listing for File mesh_geometry.cpp
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_resources_mesh_geometry.cpp>` (``portal\engine\resources\resources\mesh_geometry.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "mesh_geometry.h"
   
   namespace portal
   {
   MeshGeometry::MeshGeometry(const StringId& id, const resources::MeshGeometryData& geometry)
       : Resource(id),
         geometry(geometry)
   {
   }
   
   const std::shared_ptr<renderer::vulkan::AllocatedBuffer>& MeshGeometry::get_index_buffer() const
   {
       return geometry.index_buffer;
   }
   
   const vk::DeviceAddress& MeshGeometry::get_vertex_buffer_address() const
   {
       return geometry.vertex_buffer_address;
   }
   
   const resources::MeshGeometryData& MeshGeometry::get_geometry() const
   {
       return geometry;
   }
   
   const std::vector<resources::MeshGeometryData::Submesh>& MeshGeometry::get_submeshes() const
   {
       return geometry.submeshes;
   }
   } // portal
