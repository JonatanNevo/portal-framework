
.. _program_listing_file_portal_engine_renderer_descriptors_descriptor.h:

Program Listing for File descriptor.h
=====================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptors_descriptor.h>` (``portal\engine\renderer\descriptors\descriptor.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/core/buffer.h"
   #include "portal/engine/renderer/descriptors/descriptor_types.h"
   #include "portal/engine/renderer/renderer_resource.h"
   
   namespace portal::renderer
   {
   class BufferDescriptor : public RendererResource
   {
   public:
       ~BufferDescriptor() override = default;
       explicit BufferDescriptor(const StringId& id, const DescriptorResourceType type) : RendererResource(id), type(type) {};
   
       virtual void set_data(Buffer data, size_t offset = 0) = 0;
       virtual const Buffer& get_data() const = 0;
   
       DescriptorResourceType get_type() const { return type; };
   
   protected:
       DescriptorResourceType type;
   };
   }
