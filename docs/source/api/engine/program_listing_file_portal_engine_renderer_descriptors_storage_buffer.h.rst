
.. _program_listing_file_portal_engine_renderer_descriptors_storage_buffer.h:

Program Listing for File storage_buffer.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptors_storage_buffer.h>` (``portal\engine\renderer\descriptors\storage_buffer.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/renderer/descriptors/descriptor.h"
   #include "portal/core/strings/string_id.h"
   
   namespace portal::renderer
   {
   struct StorageBufferProperties
   {
       size_t size;
       bool gpu_only = true;
       StringId debug_name;
   };
   
   class StorageBuffer : public BufferDescriptor
   {
   public:
       explicit StorageBuffer(const StringId& id) : BufferDescriptor(id, DescriptorResourceType::StorageBuffer) {};
   
       virtual void resize(size_t new_size) = 0;
   };
   
   class StorageBufferSet : public BufferDescriptor
   {
   public:
       explicit StorageBufferSet(const StringId& id) : BufferDescriptor(id, DescriptorResourceType::StorageBufferSet) {};
   
       virtual Reference<StorageBuffer> get(size_t index) = 0;
   
       virtual void set(const Reference<StorageBuffer>& buffer, size_t index) = 0;
   };
   }
