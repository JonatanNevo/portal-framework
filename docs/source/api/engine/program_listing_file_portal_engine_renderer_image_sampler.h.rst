
.. _program_listing_file_portal_engine_renderer_image_sampler.h:

Program Listing for File sampler.h
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_image_sampler.h>` (``portal\engine\renderer\image\sampler.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   namespace portal::renderer
   {
   class Sampler
   {
   public:
       virtual ~Sampler() = default;
   
       [[nodiscard]] virtual const SamplerProperties& get_prop() const = 0;
   };
   }
