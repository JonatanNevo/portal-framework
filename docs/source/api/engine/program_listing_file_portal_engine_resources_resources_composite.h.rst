
.. _program_listing_file_portal_engine_resources_resources_composite.h:

Program Listing for File composite.h
====================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_resources_composite.h>` (``portal\engine\resources\resources\composite.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/renderer/image/texture.h"
   #include "portal/engine/renderer/material/material.h"
   #include "portal/engine/resources/resource_reference.h"
   #include "portal/engine/resources/resources/mesh_geometry.h"
   #include "portal/engine/resources/resources/resource.h"
   #include "portal/engine/scene/scene.h"
   
   
   namespace portal
   {
   class Composite final : public Resource
   {
   public:
       explicit Composite(const StringId& id) : Resource(id) {}
   
       std::optional<ResourceReference<renderer::Texture>> get_texture(const StringId& resource_id) const;
       std::optional<ResourceReference<renderer::Material>> get_material(const StringId& resource_id) const;
       std::optional<ResourceReference<MeshGeometry>> get_mesh(const StringId& resource_id) const;
       std::optional<ResourceReference<Scene>> get_scene(const StringId& resource_id) const;
   
       auto list_scenes() const;
   
       void set_resource(ResourceType type, const StringId& resource_id, const ResourceReference<Resource>& resource);
   
   private:
       // TODO: change to weak resource reference
       std::unordered_map<ResourceType, std::unordered_map<StringId, ResourceReference<Resource>>> resources{};
   };
   } // portal
