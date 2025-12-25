
.. _program_listing_file_portal_engine_renderer_surface_surface.h:

Program Listing for File surface.h
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_surface_surface.h>` (``portal\engine\renderer\surface\surface.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "llvm/ADT/SmallVector.h"
   #include "portal/core/strings/string_id.h"
   #include "portal/engine/reference.h"
   #include "portal/engine/renderer/surface/capabilities.h"
   
   namespace portal
   {
   class Window;
   }
   
   namespace portal::renderer
   {
   struct SurfaceProperties
   {
       StringId debug_name;
   
       size_t min_frames_in_flight = 3;
       // Passing `std::nullopt` for headless surface
       std::optional<std::reference_wrapper<Window>> window;
   };
   
   enum class SurfaceType
   {
       Normal,
       Headless
   };
   
   class Surface
   {
   public:
       explicit Surface(SurfaceProperties properties);
   
       virtual ~Surface() = default;
   
       [[nodiscard]] virtual const SurfaceCapabilities& get_capabilities() const = 0;
   
       [[nodiscard]] virtual glm::ivec2 get_extent() const = 0;
   
       [[nodiscard]] virtual SurfaceType get_type() const = 0;
       [[nodiscard]] virtual size_t get_min_frames_in_flight() const = 0;
   
   protected:
       SurfaceProperties properties;
   };
   } // portal
