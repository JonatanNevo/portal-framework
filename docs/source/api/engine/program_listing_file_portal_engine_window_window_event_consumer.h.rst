
.. _program_listing_file_portal_engine_window_window_event_consumer.h:

Program Listing for File window_event_consumer.h
================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_window_window_event_consumer.h>` (``portal\engine\window\window_event_consumer.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/engine/window/window.h"
   
   namespace portal
   {
   class WindowEventConsumer
   {
   public:
       virtual ~WindowEventConsumer() = default;
   
       virtual void on_resize(WindowExtent extent) = 0;
       virtual void on_focus(bool focused) = 0;
       virtual void on_close() = 0;
   };
   }
