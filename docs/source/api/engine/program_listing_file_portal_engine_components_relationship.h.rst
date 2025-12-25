
.. _program_listing_file_portal_engine_components_relationship.h:

Program Listing for File relationship.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_components_relationship.h>` (``portal\engine\components\relationship.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <portal/engine/ecs/entity.h>
   
   namespace portal
   {
   struct RelationshipComponent
   {
       // Children
       size_t children;
       Entity first = null_entity;
       Entity prev = null_entity;
       Entity next = null_entity;
   
       // Parent
       Entity parent = null_entity;
   };
   }
