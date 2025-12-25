
.. _program_listing_file_portal_core_events_event_handler.h:

Program Listing for File event_handler.h
========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_events_event_handler.h>` (``portal\core\events\event_handler.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/core/events/event.h"
   
   namespace portal
   {
   // TODO: use concept instead?
   
   class EventHandler
   {
   public:
       virtual ~EventHandler() = default;
   
       virtual void on_event(Event& event) = 0;
   };
   }
