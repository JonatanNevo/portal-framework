//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <portal/core/jobs/job.h>
#include "portal/core/log.h"
#include "portal/core/jobs/scheduler.h"

#include "common.h"

namespace portal
{

// ============================================================================
// Section 2: Counter (Job Completion Tracking)
// ============================================================================

// ============================================================================
// Section 2.1: Counter Basic Operations
// ============================================================================

Job<> simple_counter_job()
{
    co_await SuspendJob();
    co_return;
}

Job<> job_that_suspends(int& marker)
{
    marker = 1;
    co_await SuspendJob();
    marker = 2;
    co_return;
}

TEST_F(JobTest, CounterIncrementOnDispatchJobs)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    // Counter should start at 0
    EXPECT_EQ(counter.count.load(), 0);

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());

    // Dispatch with counter
    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

    // Counter should be incremented to 3
    EXPECT_EQ(counter.count.load(), 3) << "Counter should increment by number of dispatched jobs";
}

TEST_F(JobTest, CounterDecrementOnJobCompletion)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());

    // Dispatch with counter
    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

    EXPECT_EQ(counter.count.load(), 2);

    // Execute one job to completion
    auto state = scheduler.worker_thread_iteration();
    while (state == jobs::Scheduler::WorkerIterationState::FilledCache)
    {
        state = scheduler.worker_thread_iteration();
    }

    // Counter should have decremented
    EXPECT_LT(counter.count.load(), 2) << "Counter should decrement when jobs complete (FinalizeJob)";
}

TEST_F(JobTest, CounterReachesZeroAfterAllJobsComplete)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());

    // Dispatch with counter
    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

    EXPECT_EQ(counter.count.load(), 3);

    // Execute all jobs to completion
    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        scheduler.worker_thread_iteration();
    }

    // Counter should be back to 0
    EXPECT_EQ(counter.count.load(), 0) << "Counter should reach zero after all jobs complete";
}

TEST_F(JobTest, CounterNotModifiedWhenJobSuspends)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    int marker = 0;
    std::vector<Job<>> jobs;
    jobs.push_back(job_that_suspends(marker));

    // Dispatch with counter
    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

    EXPECT_EQ(counter.count.load(), 1);

    // Execute job until it suspends (marker will be 1)
    while (marker == 0)
    {
        scheduler.worker_thread_iteration();
    }

    // Job has suspended but not completed
    EXPECT_EQ(marker, 1) << "Job should have suspended";
    EXPECT_EQ(counter.count.load(), 1) << "Counter should NOT decrement when job suspends (only on finalize)";

    // Now complete the job
    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        scheduler.worker_thread_iteration();
    }

    EXPECT_EQ(marker, 2) << "Job should have completed";
    EXPECT_EQ(counter.count.load(), 0) << "Counter should decrement only when job finalizes";
}

} // namespace portal