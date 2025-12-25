
.. _program_listing_file_portal_platform_core_linux_linux_thread.h:

Program Listing for File linux_thread.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_platform_core_linux_linux_thread.h>` (``portal\platform\core\linux\linux_thread.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include "portal/core/concurrency/thread_base.h"
   
   namespace portal
   {
   class LinuxThread final : public ThreadBase
   {
   public:
       template <typename F, typename... Args>
       explicit LinuxThread(const ThreadSpecification& spec, F&& f, Args&&... args)
           : ThreadBase(spec)
       {
           auto callable = make_callable(std::forward<F>(f), std::forward<Args>(args)...);
   
           thread = std::jthread(
               [
                   name = spec.name,
                   affinity = spec.affinity,
                   priority = spec.priority,
                   core = spec.core,
                   callable = std::move(callable)
               ](std::stop_token st) mutable
               {
                   set_name(name);
                   set_affinity(affinity, core);
                   set_priority(priority);
   
                   callable(st);
               }
           );
       }
   
   protected:
       static void set_name(const std::string& name);
       static void set_affinity(ThreadAffinity affinity, uint16_t core);
       static void set_priority(ThreadPriority priority);
   };
   
   using Thread = LinuxThread;
   }
