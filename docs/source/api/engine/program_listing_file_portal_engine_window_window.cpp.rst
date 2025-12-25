
.. _program_listing_file_portal_engine_window_window.cpp:

Program Listing for File window.cpp
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_window_window.cpp>` (``portal\engine\window\window.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "window.h"
   
   namespace portal
   {
   Window::Window(const WindowProperties& properties, const CallbackConsumers& consumers) : properties(properties), consumers(consumers) {}
   
   WindowExtent Window::resize(const WindowExtent& requested_extent)
   {
       if (properties.resizeable)
       {
           properties.extent.width = requested_extent.width;
           properties.extent.height = requested_extent.height;
       }
   
       return properties.extent;
   }
   
   float Window::get_content_scale_factor() const
   {
       return 1.0f;
   }
   }
