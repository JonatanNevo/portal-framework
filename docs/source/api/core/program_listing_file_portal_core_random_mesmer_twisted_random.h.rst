
.. _program_listing_file_portal_core_random_mesmer_twisted_random.h:

Program Listing for File mesmer_twisted_random.h
================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_random_mesmer_twisted_random.h>` (``portal\core\random\mesmer_twisted_random.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/core/random/random.h"
   
   namespace portal
   {
   class MesmerTwistedRandom final : public Random
   {
   public:
       void init() override;
       void init(uint32_t seed) override;
   
       uint32_t get_uint() override;
       uint32_t get_uint(uint32_t min, uint32_t max) override;
   
       float get_float() override;
       float get_float(float min, float max) override;
   
   private:
       std::mt19937 engine;
       std::uniform_int_distribution<std::mt19937::result_type> int_distribution;
       std::uniform_real_distribution<float> float_distribution;
   };
   } // portal
