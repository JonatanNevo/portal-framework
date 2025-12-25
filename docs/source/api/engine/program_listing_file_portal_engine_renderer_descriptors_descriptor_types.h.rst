
.. _program_listing_file_portal_engine_renderer_descriptors_descriptor_types.h:

Program Listing for File descriptor_types.h
===========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_descriptors_descriptor_types.h>` (``portal\engine\renderer\descriptors\descriptor_types.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <cstdint>
   
   namespace portal::renderer
   {
   enum class DescriptorType : uint8_t
   {
       Unknown,
       Sampler,
       CombinedImageSampler,
       SampledImage,
       StorageImage,
       UniformTexelBuffer,
       StorageTexelBuffer,
       UniformBuffer,
       StorageBuffer,
       UniformBufferDynamic,
       StorageBufferDynamic,
       InputAttachment,
       AccelerationStructure,
       InlineUniformBlock,
   };
   
   enum class DescriptorResourceType: uint8_t
   {
       Unknown,
       UniformBuffer,
       UniformBufferSet,
       StorageBuffer,
       StorageBufferSet,
       Texture,
       TextureCube,
       Image
   };
   }
