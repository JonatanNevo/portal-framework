
.. _program_listing_file_portal_engine_renderer_device_device.h:

Program Listing for File device.h
=================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_device_device.h>` (``portal\engine\renderer\device\device.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   namespace portal::renderer
   {
   class Device
   {
   public:
       virtual ~Device() = default;
   
       virtual void wait_idle() const = 0;
   };
   } // portal
