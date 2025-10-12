//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "job.h"

#include "portal/core/debug/assert.h"
#include "portal/core/jobs/scheduler.h"

namespace portal
{

void SuspendJob::await_suspend(const std::coroutine_handle<JobPromise> handle) noexcept
{
    // At this point the coroutine pointed to by handle has been fully suspended. This is guaranteed by the c++ standard.

    auto& [scheduler, counter] = handle.promise();

    // Put the current coroutine to the back of the scheduler queue as it has been fully suspended at this point.
    // We are pausing the job so no need to pass on the counter
    scheduler->dispatch_job(static_cast<Job>(handle), nullptr);

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

        const auto next_handle = scheduler->pop_job();
        if (next_handle)
        {
            PORTAL_ASSERT(!next_handle.done(), "Job is already done");
            next_handle.resume();
        }
    }

    // Note: Once we drop off here, control will return to where the resume()  command that brought us here was issued.
}

void FinalizeJob::await_suspend(const std::coroutine_handle<JobPromise> handle) noexcept
{
    const auto counter = handle.promise().counter;
    if (counter)
    {
        const auto value = counter->count.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (value == 0)
        {
            counter->blocking.clear(std::memory_order_release);
            counter->blocking.notify_all();
        }
    }

    handle.destroy();
}
}
