
.. _program_listing_file_portal_engine_renderer_descriptors_descriptor_input.cpp:

Program Listing for File descriptor_input.cpp
=============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptors_descriptor_input.cpp>` (``portal\engine\renderer\descriptors\descriptor_input.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "descriptor_input.h"
   #include "portal/engine/renderer/descriptors/uniform_buffer.h"
   #include "portal/engine/renderer/descriptors/storage_buffer.h"
   #include "portal/engine/renderer/image/texture.h"
   #include "portal/engine/renderer/image/image.h"
   
   
   namespace portal::renderer
   {
   DescriptorInput::DescriptorInput(const Reference<UniformBuffer>& buffer) : type(DescriptorResourceType::UniformBuffer), input({1, buffer})
   {}
   
   DescriptorInput::DescriptorInput(const Reference<UniformBufferSet>& buffer) : type(DescriptorResourceType::UniformBufferSet), input({1, buffer})
   {
   }
   
   DescriptorInput::DescriptorInput(const Reference<StorageBuffer>& buffer) : type(DescriptorResourceType::StorageBuffer), input({1, buffer})
   {
   }
   
   DescriptorInput::DescriptorInput(const Reference<StorageBufferSet>& buffer) : type(DescriptorResourceType::StorageBufferSet), input({1, buffer})
   {
   }
   
   DescriptorInput::DescriptorInput(const Reference<Texture>& texture) : type(DescriptorResourceType::Texture), input({1, texture})
   {
   }
   
   DescriptorInput::DescriptorInput(const Reference<Image>& texture) : type(DescriptorResourceType::Image), input({1, texture})
   {
   }
   
   void DescriptorInput::set(const Reference<UniformBuffer>& buffer, const size_t index)
   {
       type = DescriptorResourceType::UniformBuffer;
       input[index] = buffer;
   }
   
   void DescriptorInput::set(const Reference<UniformBufferSet>& buffer, const size_t index)
   {
       type = DescriptorResourceType::UniformBufferSet;
       input[index] = buffer;
   }
   
   void DescriptorInput::set(const Reference<StorageBuffer>& buffer, const size_t index)
   {
       type = DescriptorResourceType::StorageBuffer;
       input[index] = buffer;
   }
   
   void DescriptorInput::set(const Reference<StorageBufferSet>& buffer, const size_t index)
   {
       type = DescriptorResourceType::StorageBufferSet;
       input[index] = buffer;
   }
   
   void DescriptorInput::set(const Reference<Texture>& texture, const size_t index)
   {
       type = DescriptorResourceType::Texture;
       input[index] = texture;
   }
   
   void DescriptorInput::set(const Reference<Image>& image, const size_t index)
   {
       type = DescriptorResourceType::Image;
       input[index] = image;
   }
   
   void DescriptorInput::set(const Reference<ImageView>& image, const size_t index)
   {
       type = DescriptorResourceType::Image;
       input[index] = image;
   }
   }
