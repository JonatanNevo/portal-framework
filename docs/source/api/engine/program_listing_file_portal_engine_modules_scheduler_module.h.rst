
.. _program_listing_file_portal_engine_modules_scheduler_module.h:

Program Listing for File scheduler_module.h
===========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_modules_scheduler_module.h>` (``portal\engine\modules\scheduler_module.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include "portal/application/modules/module.h"
   #include "portal/core/jobs/scheduler.h"
   
   namespace portal
   {
   class SchedulerModule final : public Module<>
   {
   public:
       explicit SchedulerModule(ModuleStack& stack, int32_t num_workers);
   
       [[nodiscard]] const jobs::Scheduler& get_scheduler() const { return scheduler; }
       [[nodiscard]] jobs::Scheduler& get_scheduler() { return scheduler; }
   
   private:
       jobs::Scheduler scheduler;
   };
   } // portal
