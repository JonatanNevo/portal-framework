
.. _program_listing_file_portal_engine_modules_scheduler_module.cpp:

Program Listing for File scheduler_module.cpp
=============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_modules_scheduler_module.cpp>` (``portal\engine\modules\scheduler_module.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "scheduler_module.h"
   
   namespace portal
   {
   SchedulerModule::SchedulerModule(ModuleStack& stack, const int32_t num_workers) : Module<>(stack, STRING_ID("Scheduler")), scheduler(num_workers) {}
   } // portal
