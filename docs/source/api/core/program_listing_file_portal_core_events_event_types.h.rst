
.. _program_listing_file_portal_core_events_event_types.h:

Program Listing for File event_types.h
======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_events_event_types.h>` (``portal\core\events\event_types.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <cstdint>
   #include <portal/core/flags.h>
   
   namespace portal
   {
   enum class EventType: uint8_t
   {
       Unknown = 0,
       // Window Events
       WindowClose,
       WindowMinimize,
       WindowResize,
       WindowFocus,
       WindowLostFocus,
       WindowMoved,
       WindowTitleBarHit,
       // Input Events
       KeyPressed,
       KeyReleased,
       KeyRepeat,
       MouseMoved,
       MouseScrolled,
       SetMouseCursor
   };
   
   enum class EventCategoryBits: uint8_t
   {
       Unknown = 0,
       Window,
       Input
   };
   
   using EventCategory = Flags<EventCategoryBits>;
   
   template <>
   struct FlagTraits<EventCategoryBits>
   {
       using enum EventCategoryBits;
   
       static constexpr bool is_bitmask = true;
       static constexpr auto all_flags = Window | Input;
   };
   }
