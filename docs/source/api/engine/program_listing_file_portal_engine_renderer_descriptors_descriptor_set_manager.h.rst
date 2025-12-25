
.. _program_listing_file_portal_engine_renderer_descriptors_descriptor_set_manager.h:

Program Listing for File descriptor_set_manager.h
=================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptors_descriptor_set_manager.h>` (``portal\engine\renderer\descriptors\descriptor_set_manager.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/core/strings/string_id.h"
   #include "portal/engine/renderer/descriptors/descriptor_input.h"
   #include "portal/engine/resources/resource_reference.h"
   
   namespace portal::renderer
   {
   class ShaderVariant;
   
   struct DescriptorSetManagerProperties
   {
       Reference<ShaderVariant> shader;
       StringId debug_name;
   
       // Set range to manage
       size_t start_set = 0;
       size_t end_set = std::numeric_limits<size_t>::max();
   
       Reference<Texture> default_texture;
   
       // TODO: get from global config
       size_t frame_in_flights;
   };
   
   class DescriptorSetManager
   {
   public:
       virtual ~DescriptorSetManager() = default;
   
       virtual void set_input(StringId name, const Reference<UniformBufferSet>& buffer) = 0;
       virtual void set_input(StringId name, const Reference<UniformBuffer>& buffer) = 0;
       virtual void set_input(StringId name, const Reference<StorageBufferSet>& buffer) = 0;
       virtual void set_input(StringId name, const Reference<StorageBuffer>& buffer) = 0;
       virtual void set_input(StringId name, const Reference<Texture>& texture) = 0;
       virtual void set_input(StringId name, const Reference<Image>& image) = 0;
       virtual void set_input(StringId name, const Reference<ImageView>& image) = 0;
   
       template <typename T>
       Reference<T> get_input(const StringId name)
       {
           return reference_cast<T, RendererResource>(get_input(name));
       }
   
       virtual Reference<RendererResource> get_input(StringId name) = 0;
   
       virtual bool is_invalidated(size_t set, size_t binding_index) const = 0;
       virtual bool validate() = 0;
       virtual void bake() = 0;
   };
   }
