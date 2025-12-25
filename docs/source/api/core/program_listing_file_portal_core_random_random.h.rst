
.. _program_listing_file_portal_core_random_random.h:

Program Listing for File random.h
=================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_random_random.h>` (``portal\core\random\random.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   // TODO: Read and implement:
   // https://prng.di.unimi.it/
   // https://github.com/imneme/pcg-cpp/blob/master/include/pcg_random.hpp
   
   #pragma once
   #include <cstdint>
   #include <random>
   
   #include "portal/core/glm.h"
   
   namespace portal
   {
   class Random
   {
   public:
       virtual ~Random() = default;
   
       virtual void init() = 0;
       virtual void init(uint32_t seed) = 0;
       virtual uint32_t get_uint() = 0;
       virtual uint32_t get_uint(uint32_t min, uint32_t max) = 0;
       virtual float get_float() = 0;
       virtual float get_float(float min, float max) = 0;
   
       glm::vec3 get_vec3();
       glm::vec3 get_vec3(float min, float max);
   };
   } // portal
