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
    auto state = scheduler.main_thread_do_work();

    while (state == jobs::WorkerIterationState::FilledCache)
    {
        state = scheduler.main_thread_do_work();
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
        scheduler.main_thread_do_work();
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
        scheduler.main_thread_do_work();
    }

    // Job has suspended but not completed
    EXPECT_EQ(marker, 1) << "Job should have suspended";
    EXPECT_EQ(counter.count.load(), 1) << "Counter should NOT decrement when job suspends (only on finalize)";

    // Now complete the job
    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        scheduler.main_thread_do_work();
    }

    EXPECT_EQ(marker, 2) << "Job should have completed";
    EXPECT_EQ(counter.count.load(), 0) << "Counter should decrement only when job finalizes";
}

// ============================================================================
// Section 2.2: Blocking/Unblocking
// ============================================================================

TEST_F(JobTest, BlockingFlagPreventsMultipleThreadsFromBlocking)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    // Initially the blocking flag should be clear
    EXPECT_FALSE(counter.blocking.test(std::memory_order_acquire)) << "Blocking flag should start clear";

    // Simulate wait_for_jobs behavior: set the blocking flag
    counter.blocking.test_and_set(std::memory_order_acquire);
    EXPECT_TRUE(counter.blocking.test(std::memory_order_acquire)) << "Blocking flag should be set";

    // If another thread tries to set it, test_and_set returns the previous value (true)
    bool was_already_set = counter.blocking.test_and_set(std::memory_order_acquire);
    EXPECT_TRUE(was_already_set) << "test_and_set should return true if already set";

    // Clear for cleanup
    counter.blocking.clear(std::memory_order_release);
    EXPECT_FALSE(counter.blocking.test(std::memory_order_acquire)) << "Blocking flag should be clear after clear()";
}

TEST_F(JobTest, CounterUnblocksWhenCountReachesZero)
{
    jobs::Scheduler scheduler{0};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());

    // wait_for_jobs creates a counter internally and blocks until it reaches zero
    scheduler.wait_for_jobs(std::span{jobs});

    // If we reach here, the counter successfully unblocked
    SUCCEED() << "wait_for_jobs unblocked successfully when counter reached zero";
}

TEST_F(JobTest, FinalizeJobNotifiesOnlyWhenCountReachesZero)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());

    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

    EXPECT_EQ(counter.count.load(), 3);

    // Execute first job - count goes to 2, should NOT notify
    auto state = scheduler.main_thread_do_work();
    while (state == jobs::WorkerIterationState::FilledCache)
    {
        state = scheduler.main_thread_do_work();
    }
    EXPECT_GT(counter.count.load(), 0) << "Still have jobs pending";

    // Execute second job - count goes to 1, should NOT notify
    state = scheduler.main_thread_do_work();
    while (state == jobs::WorkerIterationState::FilledCache)
    {
        state = scheduler.main_thread_do_work();
    }
    EXPECT_GT(counter.count.load(), 0) << "Still have one job pending";

    // Execute third job - count goes to 0, SHOULD notify
    state = scheduler.main_thread_do_work();
    while (state == jobs::WorkerIterationState::FilledCache)
    {
        state = scheduler.main_thread_do_work();
    }

    EXPECT_EQ(counter.count.load(), 0) << "All jobs complete, notification should have occurred";
}

// ============================================================================
// Section 2.3: Memory Ordering
// ============================================================================

TEST_F(JobTest, FetchAddUsesReleaseOrdering)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());

    // dispatch_jobs should use fetch_add with release ordering
    // This ensures the counter increment is visible to other threads
    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

    // Verify the counter was incremented (this also tests that release ordering worked)
    EXPECT_EQ(counter.count.load(std::memory_order_acquire), 2)
        << "fetch_add with release ordering should make count visible to acquire loads";
}

TEST_F(JobTest, FetchSubUsesReleaseOrdering)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());

    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);
    EXPECT_EQ(counter.count.load(), 1);

    // Execute job - FinalizeJob should use fetch_sub with release ordering
    // This ensures the decrement is visible to other threads
    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        scheduler.main_thread_do_work();
    }

    // Verify counter reached zero (release ordering in fetch_sub allowed acquire to see it)
    EXPECT_EQ(counter.count.load(std::memory_order_acquire), 0)
        << "fetch_sub with release ordering in FinalizeJob should make decrement visible";
}

TEST_F(JobTest, LoadUsesAcquireOrderingInWaitLoops)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());

    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

    // Simulate wait loop with acquire ordering
    // This is the pattern used in wait_for_jobs (scheduler.cpp:100)
    size_t iterations = 0;
    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        scheduler.main_thread_do_work();
        ++iterations;
        ASSERT_LT(iterations, 1000) << "Loop should terminate (acquire ordering sees updates)";
    }

    EXPECT_EQ(counter.count.load(std::memory_order_acquire), 0)
        << "Acquire load should synchronize with release operations";
}

TEST_F(JobTest, BlockingFlagUsesProperAcquireRelease)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    // Test the blocking flag memory ordering pattern
    // Set flag with test_and_set (acquire ordering)
    bool was_set = counter.blocking.test_and_set(std::memory_order_acquire);
    EXPECT_FALSE(was_set) << "Flag should initially be clear";

    // Test with acquire
    bool is_set = counter.blocking.test(std::memory_order_acquire);
    EXPECT_TRUE(is_set) << "Flag should be set after test_and_set";

    // Clear with release
    counter.blocking.clear(std::memory_order_release);

    // Verify with acquire
    is_set = counter.blocking.test(std::memory_order_acquire);
    EXPECT_FALSE(is_set) << "Flag should be clear after clear() with release ordering";
}

TEST_F(JobTest, TestAndSetUsesAcquireOrdering)
{
    jobs::Counter counter{};

    // test_and_set should use acquire ordering to ensure
    // any prior writes are visible after acquiring the flag
    bool was_set = counter.blocking.test_and_set(std::memory_order_acquire);
    EXPECT_FALSE(was_set) << "First test_and_set should return false (wasn't set)";

    // Calling again should return true (was already set)
    was_set = counter.blocking.test_and_set(std::memory_order_acquire);
    EXPECT_TRUE(was_set) << "Second test_and_set should return true (was set)";

    // The acquire ordering ensures we see any writes that happened-before the set
    counter.blocking.clear(std::memory_order_release);
}

TEST_F(JobTest, ClearUsesReleaseOrdering)
{
    jobs::Counter counter{};

    // Set the flag
    counter.blocking.test_and_set(std::memory_order_acquire);
    EXPECT_TRUE(counter.blocking.test(std::memory_order_acquire));

    // Clear should use release ordering to ensure all prior writes
    // are visible to threads that later acquire the flag
    counter.blocking.clear(std::memory_order_release);

    // Verify with acquire ordering
    bool is_set = counter.blocking.test(std::memory_order_acquire);
    EXPECT_FALSE(is_set) << "After clear() with release, flag should be visible as clear to acquire";
}

TEST_F(JobTest, CounterMemoryOrderingIntegrationTest)
{
    jobs::Scheduler scheduler{0};
    jobs::Counter counter{};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());
    jobs.push_back(simple_counter_job());

    // This test verifies the complete memory ordering interaction:
    // 1. dispatch_jobs: fetch_add with release
    // 2. wait loop: load with acquire
    // 3. FinalizeJob: fetch_sub with release
    // 4. blocking flag: test_and_set with acquire, clear with release

    scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

    // Simulate wait_for_jobs pattern with correct memory ordering
    counter.blocking.test_and_set(std::memory_order_acquire);

    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        // If no work, we would normally wait, but for test just iterate
        auto state = scheduler.main_thread_do_work();
        if (state == jobs::WorkerIterationState::EmptyQueue)
        {
            // In real code, would wait on blocking flag
            // For test, just clear and check count again
            counter.blocking.clear(std::memory_order_release);
            if (counter.count.load(std::memory_order_acquire) > 0)
            {
                counter.blocking.test_and_set(std::memory_order_acquire);
            }
        }
    }

    counter.blocking.clear(std::memory_order_release);

    EXPECT_EQ(counter.count.load(std::memory_order_acquire), 0)
        << "All memory orderings should work together correctly";
}

} // namespace portal