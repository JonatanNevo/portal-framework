
.. _program_listing_file_portal_input_input_event_consumer.h:

Program Listing for File input_event_consumer.h
===============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_input_input_event_consumer.h>` (``portal\input\input_event_consumer.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <optional>
   #include <portal/core/glm.h>
   #include "portal/input/input_types.h"
   
   namespace portal
   {
   class InputEventConsumer
   {
   public:
       virtual ~InputEventConsumer() = default;
   
       virtual void report_key_action(Key key, KeyState state, std::optional<KeyModifierFlag> modifiers);
       virtual void report_axis_change(Axis axis, glm::vec2 value);
   };
   }
