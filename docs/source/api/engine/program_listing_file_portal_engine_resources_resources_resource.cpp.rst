
.. _program_listing_file_portal_engine_resources_resources_resource.cpp:

Program Listing for File resource.cpp
=====================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_resources_resource.cpp>` (``portal\engine\resources\resources\resource.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "resource.h"
   
   namespace portal
   {
   bool Resource::operator==(const Resource& other) const
   {
       return id == other.id;
   }
   } // portal
