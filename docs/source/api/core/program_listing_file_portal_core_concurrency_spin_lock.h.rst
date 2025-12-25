
.. _program_listing_file_portal_core_concurrency_spin_lock.h:

Program Listing for File spin_lock.h
====================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_concurrency_spin_lock.h>` (``portal\core\concurrency\spin_lock.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <atomic>
   #include <thread>
   
   namespace portal
   {
   class SpinLock
   {
   public:
       SpinLock() = default;
   
       bool try_lock()
       {
           // Use an acquire fence to ensure all subsequent reads by this thread will be valid
           const bool already_locked = locked.test_and_set(std::memory_order_acquire);
           return !already_locked;
       }
   
       void lock()
       {
           unsigned backoff = 1;
   
           if (try_lock())
               return;
   
           // Exponential back-off strategy
           while (true)
           {
               for (unsigned i = 0; i < backoff; ++i)
                   std::this_thread::yield();
   
               if (try_lock())
                   return;
   
               // Increase back-off time (with a reasonable upper limit)
               if (backoff < 1024)
                   backoff *= 2;
           }
       }
   
       void unlock() noexcept
       {
           // Use release semantics to ensure that all prior write have been fully commited before we unlock
           locked.clear(std::memory_order_release);
       }
   
   private:
       std::atomic_flag locked = ATOMIC_FLAG_INIT;
   };
   }
