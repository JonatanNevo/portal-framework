
.. _program_listing_file_portal_engine_resources_utils.h:

Program Listing for File utils.h
================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_resources_utils.h>` (``portal\engine\resources\utils.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/engine/resources/resource_types.h"
   #include "portal/engine/resources/resources/resource.h"
   
   
   namespace portal::utils
   {
   template <class T>
   ResourceType to_resource_type()
   {
       return T::static_type();
   }
   
   ResourceType to_resource_type(const Resource& resource);
   }
