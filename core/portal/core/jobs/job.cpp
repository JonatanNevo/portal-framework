//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "job.h"

#include "portal/core/debug/assert.h"
#include "portal/core/jobs/scheduler.h"

namespace portal
{

constexpr static size_t JOB_POOL_SIZE = 1024;
#if defined(PORTAL_TEST)
static BucketPoolAllocator<512, JOB_POOL_SIZE, SpinLock, true> g_job_promise_allocator{};
#else
static BucketPoolAllocator<512, JOB_POOL_SIZE> g_job_promise_allocator{};
#endif

static std::pmr::synchronized_pool_resource g_job_result_allocator{};

void SuspendJob::await_suspend(const std::coroutine_handle<> handle) noexcept
{
    // At this point the coroutine pointed to by handle has been fully suspended. This is guaranteed by the c++ standard.

    auto job_promise_handler = std::coroutine_handle<JobPromise>::from_address(handle.address());
    job_promise_handler.promise().add_switch_information(SwitchType::Pause);

    auto& [scheduler, counter, _, switch_information] = job_promise_handler.promise();

    // Put the current coroutine to the back of the scheduler queue as it has been fully suspended at this point.
    // We are pausing the job so no need to pass on the counter
    scheduler->dispatch_job({job_promise_handler}, nullptr);

    if (counter)
    {
        // We must unblock/awake the scheduling thread each time we suspend a coroutine so that the scheduling worker may pick up work again,
        // in case it had been put to sleep earlier.
        counter->blocking.clear(std::memory_order_release);
        counter->blocking.notify_all();
    }

    {
        // --- Eager Workers ---
        //
        // Eagerly try to fetch & execute the next task from the front of the
        // scheduler queue -
        // We do this so that multiple threads can share the
        // scheduling workload.
        //
        // But we can also disable that so that there is only one thread
        // that does the scheduling and removing elements from the
        // queue.

        const auto next_handle = scheduler->try_dequeue_job();
        if (next_handle)
        {
            PORTAL_ASSERT(!next_handle.done(), "Job is already done");
            next_handle.promise().add_switch_information(SwitchType::Resume);
            next_handle.resume();
        }
    }

    // Note: Once we drop off here, control will return to where the resume()  command that brought us here was issued.
}

void FinalizeJob::await_suspend(const std::coroutine_handle<> handle) noexcept
{
    auto job_promise_handler = std::coroutine_handle<JobPromise>::from_address(handle.address());
    job_promise_handler.promise().add_switch_information(SwitchType::Finish);

    const auto counter = job_promise_handler.promise().counter;
    if (counter)
    {
        const auto value = counter->count.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (value == 0)
        {
            counter->blocking.clear(std::memory_order_release);
            counter->blocking.notify_all();
        }
    }
}

JobPromise::JobPromise()
{
    switch_information.reserve(16);
}


void JobPromise::unhandled_exception() noexcept
{
    add_switch_information(SwitchType::Error);

    LOG_ERROR_TAG("Task", "Unhandled exception in task");
}

void JobPromise::add_switch_information(SwitchType type)
{
    switch_information.push_back(
        {
            std::this_thread::get_id(),
            std::chrono::system_clock::now(),
            type,
        }
        );
}

void* JobPromise::operator new([[maybe_unused]] size_t n) noexcept
{
    static size_t counter = 0;
    PORTAL_ASSERT(n <= g_job_promise_allocator.bucket_size, "Attempting to allocate more than bucket size");
    counter++;
    try
    {
        return g_job_promise_allocator.alloc();
    }
    catch (std::bad_alloc&)
    {
        return nullptr;
    }
}

void JobPromise::operator delete(void* ptr) noexcept
{
    g_job_promise_allocator.free(ptr);
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
}
