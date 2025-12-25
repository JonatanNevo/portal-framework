
.. _program_listing_file_portal_core_random_random.cpp:

Program Listing for File random.cpp
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_random_random.cpp>` (``portal\core\random\random.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "random.h"
   
   namespace portal
   {
   glm::vec3 Random::get_vec3()
   {
       return {get_float(), get_float(), get_float()};
   }
   
   glm::vec3 Random::get_vec3(const float min, const float max)
   {
       return {get_float(min, max), get_float(min, max), get_float(min, max)};
   }
   } // namespace portal
