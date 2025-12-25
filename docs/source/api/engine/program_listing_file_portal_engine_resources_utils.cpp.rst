
.. _program_listing_file_portal_engine_resources_utils.cpp:

Program Listing for File utils.cpp
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_utils.cpp>` (``portal\engine\resources\utils.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "utils.h"
   
   namespace portal::resources::utils
   {
   ResourceType to_resource_type(const Resource& resource)
   {
       return resource.static_type();
   }
   }
