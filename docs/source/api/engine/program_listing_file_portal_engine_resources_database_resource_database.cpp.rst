
.. _program_listing_file_portal_engine_resources_database_resource_database.cpp:

Program Listing for File resource_database.cpp
==============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_database_resource_database.cpp>` (``portal\engine\resources\database\resource_database.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   
   #include "resource_database.h"
   
   #include <ranges>
   
   #include "portal/core/strings/string_utils.h"
   #include "portal/engine/resources/loader/gltf_loader.h"
   
   namespace portal
   {
   void CompositeMetadata::archive(ArchiveObject& archive) const
   {
       auto* child = archive.create_child("composite");
       child->add_property("type", type);
       child->add_property("children", children);
   }
   
   CompositeMetadata CompositeMetadata::dearchive(ArchiveObject& archive)
   {
       auto* child = archive.get_object("composite");
   
       std::string type;
       std::unordered_map<std::string, SourceMetadata> children;
       child->get_property("type", type);
       child->get_property("children", children);
   
       return CompositeMetadata{children, type};
   }
   
   void TextureMetadata::archive(ArchiveObject& archive) const
   {
       auto* child = archive.create_child("texture");
       child->add_property<bool>("hdr", hdr);
       child->add_property<size_t>("width", width);
       child->add_property<size_t>("height", height);
       child->add_property("format", to_string(format));
   }
   
   TextureMetadata TextureMetadata::dearchive(ArchiveObject& archive)
   {
       auto* child = archive.get_object("texture");
       bool hdr;
       size_t width, height;
       std::string format;
       child->get_property<bool>("hdr", hdr);
       child->get_property<size_t>("width", width);
       child->get_property<size_t>("height", height);
       child->get_property("format", format);
   
       return {
           hdr,
           width,
           height,
           from_string<renderer::ImageFormat>(format)
       };
   }
   
   void MaterialMetadata::archive(ArchiveObject& archive) const
   {
       auto* child = archive.create_child("material");
       child->add_property("shader", shader.string);
   }
   
   MaterialMetadata MaterialMetadata::dearchive(ArchiveObject& archive)
   {
       auto* child = archive.get_object("material");
       std::string shader_name;
       child->get_property("shader", shader_name);
   
       return MaterialMetadata{STRING_ID(shader_name)};
   }
   
   void SourceMetadata::archive(ArchiveObject& archive) const
   {
       archive.add_property("resource_id", resource_id.string);
       archive.add_property("type", to_string(type));
       archive.add_property(
           "dependencies",
           dependencies | std::ranges::views::transform([](const auto& id) { return id.string; }) | std::ranges::to<std::vector>()
       ); // TODO: support views
   
       archive.add_property("source", source.string);
       archive.add_property("format", to_string(format));
   
       std::visit(
           [&archive](const auto& meta)
           {
               meta.archive(archive);
           },
           meta
       );
   }
   
   SourceMetadata SourceMetadata::dearchive(ArchiveObject& archive)
   {
       std::string resource_name;
       std::string type_string;
   
       std::string source;
       std::string format_string;
       std::vector<std::string> dependencies;
   
       archive.get_property<std::string>("resource_id", resource_name);
       archive.get_property<std::string>("type", type_string);
       archive.get_property<std::vector<std::string>>("dependencies", dependencies);
   
       archive.get_property<std::string>("source", source);
       archive.get_property<std::string>("format", format_string);
   
       auto string_id_view = dependencies | std::ranges::views::transform([](const auto& id) { return STRING_ID(id); });
       SourceMetadata metadata{
           STRING_ID(resource_name),
           from_string<ResourceType>(type_string),
           string_id_view | std::ranges::to<llvm::SmallVector<StringId>>(),
           STRING_ID(source),
           from_string<SourceFormat>(format_string)
       };
   
       switch (metadata.type)
       {
       case ResourceType::Texture:
           metadata.meta = TextureMetadata::dearchive(archive);
           break;
       case ResourceType::Composite:
           metadata.meta = CompositeMetadata::dearchive(archive);
           break;
       case ResourceType::Material:
           metadata.meta = MaterialMetadata::dearchive(archive);
           break;
       default:
           break;
       }
       return metadata;
   }
   } // portal
