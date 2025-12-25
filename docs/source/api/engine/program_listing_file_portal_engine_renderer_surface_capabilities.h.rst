
.. _program_listing_file_portal_engine_renderer_surface_capabilities.h:

Program Listing for File capabilities.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_surface_capabilities.h>` (``portal\engine\renderer\surface\capabilities.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <portal/core/glm.h>
   
   #include "portal/core/flags.h"
   
   namespace portal::renderer
   {
   enum class SurfaceTransformBits: uint16_t
   {
       Emtpy           = 0b000000000,
       Identity        = 0b000000001,
       Rotate90        = 0b000000010,
       Rotate180       = 0b000000100,
       Rotate270       = 0b000001000,
       Mirror          = 0b000010000,
       MirrorRotate90  = 0b000100000, // TODO: is `MirrorRotate` is a different flag from `Mirror | Rotate` ?
       MirrorRotate180 = 0b001000000,
       MirrorRotate270 = 0b010000000,
       Inherit         = 0b100000000
   };
   
   using SurfaceTransform = Flags<SurfaceTransformBits>;
   
   struct SurfaceCapabilities
   {
       size_t min_swapchain_images;
       size_t max_swapchain_images;
   
       glm::ivec2 current_extent;
       glm::ivec2 min_image_extent;
       glm::ivec2 max_image_extent;
   
       size_t max_image_array_layers;
   
       SurfaceTransform supported_transforms;
       SurfaceTransform current_transform;
   };
   }
   
   
   template <>
   struct portal::FlagTraits<portal::renderer::SurfaceTransformBits>
   {
       using enum renderer::SurfaceTransformBits;
   
       static constexpr bool is_bitmask = true;
       static constexpr auto all_flags = Identity | Rotate90 | Rotate180 | Rotate270 | Mirror | MirrorRotate90 | MirrorRotate180 | MirrorRotate270 |
           Inherit;
   };
