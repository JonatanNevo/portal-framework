//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <portal/core/jobs/job.h>
#include "portal/core/log.h"
#include "portal/core/jobs/scheduler.h"
#include <mutex>
#include <set>

#include "common.h"

namespace portal
{

// ============================================================================
// Section 5: Nested Jobs (Jobs within Jobs)
// ============================================================================

Job<> inner_coroutine(int i, int j, ExecutionTracker* tracker = nullptr)
{
    if (tracker)
    {
        tracker->record(std::format("inner_{}_{}", i, j));
    }

    co_await SuspendJob();
    co_return;
}

Job<> outer_coroutine(int i, jobs::Scheduler& scheduler, ExecutionTracker* tracker = nullptr)
{
    if (tracker)
    {
        tracker->record(std::format("outer_{}", i));
    }

    std::vector<Job<>> jobs;
    for (size_t j = 0; j < 5; ++j)
    {
        jobs.push_back(inner_coroutine(i, j, tracker));
    }

    co_await SuspendJob();
    scheduler.wait_for_jobs(std::span{jobs});

    co_return;
}

TEST_F(JobTest, JobWithinJob)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(1);
    ExecutionTracker tracker;

    std::vector<Job<>> jobs;
    for (int i = 0; i < 20; ++i)
    {
        jobs.push_back(outer_coroutine(i, scheduler, &tracker));
    }

    scheduler.wait_for_jobs(std::span{jobs});


    // Verify all outer coroutines executed
    for (int i = 0; i < 20; ++i)
    {
        EXPECT_TRUE(tracker.was_executed(std::format("outer_{}", i)))
            << "outer_" << i << " did not execute";
    }

    // Verify all inner coroutines executed (20 outer * 5 inner each = 100)
    for (int i = 0; i < 20; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            EXPECT_TRUE(tracker.was_executed(std::format("inner_{}_{}", i, j)))
                << "inner_" << i << "_" << j << " did not execute";
        }
    }

    // Verify total execution count (20 outer + 100 inner = 120)
    EXPECT_EQ(tracker.execution_count(), 120);

    // Verify execution order: each outer coroutine should execute before its inner coroutines
    for (int i = 0; i < 20; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            EXPECT_TRUE(tracker.executed_before(
                std::format("outer_{}", i),
                std::format("inner_{}_{}", i, j)))
                << "outer_" << i << " should execute before inner_" << i << "_" << j;
        }
    }
}

} // namespace portal