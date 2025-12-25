
.. _program_listing_file_portal_engine_renderer_descriptors_uniform_buffer.h:

Program Listing for File uniform_buffer.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptors_uniform_buffer.h>` (``portal\engine\renderer\descriptors\uniform_buffer.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/engine/renderer/descriptors/descriptor.h"
   
   namespace portal::renderer
   {
   class UniformBuffer : public BufferDescriptor
   {
   public:
       UniformBuffer(const StringId& id) : BufferDescriptor(id, DescriptorResourceType::UniformBuffer) {};
   };
   
   class UniformBufferSet : public BufferDescriptor
   {
   public:
       UniformBufferSet(const StringId& id) : BufferDescriptor(id, DescriptorResourceType::UniformBufferSet) {};
   
       virtual Reference<UniformBuffer> get(size_t index) = 0;
   
       virtual void set(const Reference<UniformBuffer>& buffer, size_t index) = 0;
   };
   }
