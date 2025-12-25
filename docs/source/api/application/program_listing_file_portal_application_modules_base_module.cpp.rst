
.. _program_listing_file_portal_application_modules_base_module.cpp:

Program Listing for File base_module.cpp
========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_application_modules_base_module.cpp>` (``portal\application\modules\base_module.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "base_module.h"
   
   namespace portal
   {
   void BaseModule::begin_frame(FrameContext&) {}
   
   void BaseModule::gui_update(FrameContext&) {}
   
   void BaseModule::post_update(FrameContext&) {}
   
   void BaseModule::end_frame(FrameContext&) {}
   
   void BaseModule::on_event(Event&) {}
   
   void BaseModule::update(FrameContext&) {}
   }
