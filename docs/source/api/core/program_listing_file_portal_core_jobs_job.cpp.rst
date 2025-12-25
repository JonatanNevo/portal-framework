
.. _program_listing_file_portal_core_jobs_job.cpp:

Program Listing for File job.cpp
================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_jobs_job.cpp>` (``portal\core\jobs\job.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "job.h"
   
   #include <future>
   #include <iostream>
   #include <memory_resource>
   
   
   #include "portal/core/debug/assert.h"
   #include "portal/core/jobs/scheduler.h"
   #include "portal/core/memory/pool_allocator.h"
   
   namespace portal
   {
   constexpr static size_t BUCKET_SIZE = 1024;
   constexpr static size_t JOB_POOL_SIZE = 1024;
   #if defined(PORTAL_TEST)
   static BucketPoolAllocator<BUCKET_SIZE, JOB_POOL_SIZE, SpinLock, true> g_job_promise_allocator{};
   #else
   static BucketPoolAllocator<BUCKET_SIZE, JOB_POOL_SIZE> g_job_promise_allocator{};
   #endif
   
   static std::pmr::synchronized_pool_resource g_job_result_allocator{};
   
   std::coroutine_handle<> SuspendJob::await_suspend(const std::coroutine_handle<> handle) noexcept
   {
       PORTAL_PROF_ZONE();
       // At this point the coroutine pointed to by handle has been fully suspended. This is guaranteed by the c++ standard.
   
       auto job_promise_handler = std::coroutine_handle<JobPromise>::from_address(handle.address());
       job_promise_handler.promise().add_switch_information(SwitchType::Pause);
   
       const auto& promise = job_promise_handler.promise();
       auto* counter = promise.get_counter();
       auto* scheduler = promise.get_scheduler();
   
       // Put the current coroutine to the back of the scheduler queue as it has been fully suspended at this point.
       // We are pausing the job so no need to pass on the counter
       // TODO: save the priority somewhere?
       scheduler->dispatch_job({job_promise_handler}, JobPriority::Normal, nullptr);
   
       if (counter)
       {
           // We must unblock/awake the scheduling thread each time we suspend a coroutine so that the scheduling worker may pick up work again,
           // in case it had been put to sleep earlier.
           counter->blocking.clear(std::memory_order_release);
           counter->blocking.notify_all();
       }
   
       // Return the context back to the caller (usually the thread event loop)
       return promise.get_continuation();
   }
   
   std::coroutine_handle<> FinalizeJob::await_suspend(const std::coroutine_handle<> handle) noexcept
   {
       PORTAL_PROF_ZONE();
       const auto job_promise_handler = std::coroutine_handle<JobPromise>::from_address(handle.address());
       job_promise_handler.promise().add_switch_information(SwitchType::Finish);
   
       const auto counter = job_promise_handler.promise().get_counter();
       if (counter)
       {
           const auto value = counter->count.fetch_sub(1, std::memory_order_release) - 1;
           if (value <= 0)
           {
               counter->blocking.clear(std::memory_order_release);
               counter->blocking.notify_all();
           }
       }
   
       return job_promise_handler.promise().get_continuation();
   }
   
   JobPromise::JobPromise()
   {
       switch_information.reserve(16);
   }
   
   
   void JobPromise::unhandled_exception() noexcept
   {
       PORTAL_PROF_ZONE();
       add_switch_information(SwitchType::Error);
   
       const auto exception = std::current_exception();
   
       LOG_ERROR_TAG("Task", "Unhandled exception in task");
       try
       {
           if (exception)
               std::rethrow_exception(exception);
       }
       catch (const std::exception& e)
       {
           LOG_ERROR_TAG("Task", "Exception: {}", e.what());
       }
       catch (...)
       {
           LOG_ERROR_TAG("Task", "Unknown exception");
       }
   }
   
   void JobPromise::add_switch_information(SwitchType type)
   {
       PORTAL_PROF_ZONE();
       switch_information.emplace_back(
           std::this_thread::get_id(),
           std::chrono::system_clock::now(),
           type
       );
   }
   
   void* JobPromise::operator new([[maybe_unused]] size_t n) noexcept
   {
       PORTAL_PROF_ZONE();
       // PORTAL_ASSERT(
       //     n <= g_job_promise_allocator.bucket_size,
       //     "Attempting to allocate ({}) more than bucket size ({})",
       //     n,
       //     g_job_promise_allocator.bucket_size
       //     );
   
       try
       {
           auto* ptr = std::malloc(n);
           return ptr;
   
           // return g_job_promise_allocator.alloc();
       }
       catch (std::bad_alloc&)
       {
           return nullptr;
       }
   }
   
   void JobPromise::operator delete(void* ptr) noexcept
   {
       std::free(ptr);
       // g_job_promise_allocator.free(ptr);
   }
   
   void JobPromise::set_scheduler(jobs::Scheduler* scheduler_ptr) noexcept
   {
       scheduler = scheduler_ptr;
   }
   
   void JobPromise::set_counter(jobs::Counter* counter_ptr) noexcept
   {
       counter = counter_ptr;
   }
   
   void JobPromise::set_continuation(const std::coroutine_handle<> caller) noexcept
   {
       continuation = caller;
   }
   
   size_t JobPromise::get_allocated_size() noexcept
   {
   #if defined(PORTAL_TEST)
       return g_job_promise_allocator.get_allocation_size();
   #else
       return 0;
   #endif
   }
   
   
   void* JobPromise::allocate_result(const size_t size) noexcept
   {
       return g_job_result_allocator.allocate(size);
   }
   
   void JobPromise::deallocate_result(void* ptr, const size_t size) noexcept
   {
       return g_job_result_allocator.deallocate(ptr, size);
   }
   
   bool JobPromise::JobAwaiter::await_ready() noexcept
   {
       PORTAL_PROF_ZONE();
       return !handle || handle.done();
   }
   
   std::coroutine_handle<JobPromise> JobPromise::JobAwaiter::await_suspend(std::coroutine_handle<> caller) noexcept
   {
       PORTAL_PROF_ZONE();
       handle.promise().set_continuation(caller);
       return handle;
   }
   
   
   void JobBase::set_dispatched()
   {
       dispatched = true;
   }
   
   void JobBase::set_scheduler(jobs::Scheduler* scheduler_ptr) const noexcept
   {
       handle.promise().set_scheduler(scheduler_ptr);
   }
   
   void JobBase::set_counter(jobs::Counter* counter_ptr) const noexcept
   {
       handle.promise().set_counter(counter_ptr);
   }
   }
