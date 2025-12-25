
.. _program_listing_file_portal_input_key_data.h:

Program Listing for File key_data.h
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_input_key_data.h>` (``portal\input\key_data.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "input_types.h"
   
   namespace portal
   {
   struct KeyData
   {
       Key key = Key::Invalid;
       KeyState state = KeyState::Released;
       KeyState previous_state = KeyState::Released;
   };
   }
