
.. _program_listing_file_portal_input_input_event_consumer.cpp:

Program Listing for File input_event_consumer.cpp
=================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_input_input_event_consumer.cpp>` (``portal\input\input_event_consumer.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   #include "input_event_consumer.h"
   
   #include <optional>
   
   namespace portal
   {
   void InputEventConsumer::report_key_action(Key, KeyState, std::optional<KeyModifierFlag>) {}
   
   void InputEventConsumer::report_axis_change(Axis, glm::vec2) {}
   }
