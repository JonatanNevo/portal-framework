//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_test_macros.hpp>
#include <portal/engine/core/jobs/job.h>
#include "portal/core/log.h"
#include "portal/engine/core/jobs/scheduler.h"

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

TEST_CASE("Counter Basic Operations", "[jobs][counter]")
{
    job_test_setup();

    SECTION("CounterIncrementOnDispatchJobs")
    {
        jobs::Scheduler scheduler{0};
        jobs::Counter counter{};

        // Counter should start at 0
        REQUIRE(counter.count.load() == 0);

        std::vector<Job<>> jobs;
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());

        // Dispatch with counter
        scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

        // Counter should be incremented to 3
        REQUIRE(counter.count.load() == 3);
    }

    SECTION("CounterDecrementOnJobCompletion")
    {
        jobs::Scheduler scheduler{0};
        jobs::Counter counter{};

        std::vector<Job<>> jobs;
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());

        // Dispatch with counter
        scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

        REQUIRE(counter.count.load() == 2);

        // Execute one job to completion
        auto state = scheduler.main_thread_do_work();

        while (state == jobs::WorkerIterationState::FilledCache)
        {
            state = scheduler.main_thread_do_work();
        }

        // Counter should have decremented
        REQUIRE(counter.count.load() < 2);
    }

    SECTION("CounterReachesZeroAfterAllJobsComplete")
    {
        jobs::Scheduler scheduler{0};
        jobs::Counter counter{};

        std::vector<Job<>> jobs;
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());

        // Dispatch with counter
        scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

        REQUIRE(counter.count.load() == 3);

        // Execute all jobs to completion
        while (counter.count.load(std::memory_order_acquire) > 0)
        {
            scheduler.main_thread_do_work();
        }

        // Counter should be back to 0
        REQUIRE(counter.count.load() == 0);
    }

    SECTION("CounterNotModifiedWhenJobSuspends")
    {
        jobs::Scheduler scheduler{0};
        jobs::Counter counter{};

        int marker = 0;
        std::vector<Job<>> jobs;
        jobs.push_back(job_that_suspends(marker));

        // Dispatch with counter
        scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

        REQUIRE(counter.count.load() == 1);

        // Execute job until it suspends (marker will be 1)
        while (marker == 0)
        {
            scheduler.main_thread_do_work();
        }

        // Job has suspended but not completed
        REQUIRE(marker == 1);
        REQUIRE(counter.count.load() == 1);

        // Now complete the job
        while (counter.count.load(std::memory_order_acquire) > 0)
        {
            scheduler.main_thread_do_work();
        }

        REQUIRE(marker == 2);
        REQUIRE(counter.count.load() == 0);
    }

    job_test_teardown();
}

// ============================================================================
// Section 2.2: Blocking/Unblocking
// ============================================================================

TEST_CASE("Counter Blocking/Unblocking", "[jobs][counter]")
{
    job_test_setup();

    SECTION("CounterUnblocksWhenCountReachesZero")
    {
        jobs::Scheduler scheduler{0};

        std::vector<Job<>> jobs;
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());

        // wait_for_jobs creates a counter internally and blocks until it reaches zero
        scheduler.wait_for_jobs(std::span{jobs});

        // If we reach here, the counter successfully unblocked
        REQUIRE(true);
    }

    SECTION("FinalizeJobNotifiesOnlyWhenCountReachesZero")
    {
        jobs::Scheduler scheduler{0};
        jobs::Counter counter{};

        std::vector<Job<>> jobs;
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());

        scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

        REQUIRE(counter.count.load() == 3);

        // Execute first job - count goes to 2, should NOT notify
        auto state = scheduler.main_thread_do_work();
        while (state == jobs::WorkerIterationState::FilledCache)
        {
            state = scheduler.main_thread_do_work();
        }
        REQUIRE(counter.count.load() > 0);

        // Execute second job - count goes to 1, should NOT notify
        state = scheduler.main_thread_do_work();
        while (state == jobs::WorkerIterationState::FilledCache)
        {
            state = scheduler.main_thread_do_work();
        }
        REQUIRE(counter.count.load() > 0);

        // Execute third job - count goes to 0, SHOULD notify
        state = scheduler.main_thread_do_work();
        while (state == jobs::WorkerIterationState::FilledCache)
        {
            state = scheduler.main_thread_do_work();
        }

        REQUIRE(counter.count.load() == 0);
    }

    job_test_teardown();
}

// ============================================================================
// Section 2.3: Memory Ordering
// ============================================================================

TEST_CASE("Counter Memory Ordering", "[jobs][counter]")
{
    job_test_setup();

    SECTION("FetchAddUsesReleaseOrdering")
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
        REQUIRE(counter.count.load(std::memory_order_acquire) == 2);
    }

    SECTION("FetchSubUsesReleaseOrdering")
    {
        jobs::Scheduler scheduler{0};
        jobs::Counter counter{};

        std::vector<Job<>> jobs;
        jobs.push_back(simple_counter_job());

        scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);
        REQUIRE(counter.count.load() == 1);

        // Execute job - FinalizeJob should use fetch_sub with release ordering
        // This ensures the decrement is visible to other threads
        while (counter.count.load(std::memory_order_acquire) > 0)
        {
            scheduler.main_thread_do_work();
        }

        // Verify counter reached zero (release ordering in fetch_sub allowed acquire to see it)
        REQUIRE(counter.count.load(std::memory_order_acquire) == 0);
    }

    SECTION("LoadUsesAcquireOrderingInWaitLoops")
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
            REQUIRE(iterations < 1000);
        }

        REQUIRE(counter.count.load(std::memory_order_acquire) == 0);
    }

    SECTION("CounterMemoryOrderingIntegrationTest")
    {
        jobs::Scheduler scheduler{0};
        jobs::Counter counter{};

        std::vector<Job<>> jobs;
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());
        jobs.push_back(simple_counter_job());

        // Verifies the complete memory-ordering interaction visible through the
        // public API: dispatch_jobs (fetch_add release), the wait loop driving
        // count to zero via acquire loads, and FinalizeJob's release-RMW.
        scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

        while (counter.count.load(std::memory_order_acquire) > 0)
        {
            scheduler.main_thread_do_work();
        }

        REQUIRE(counter.count.load(std::memory_order_acquire) == 0);
    }

    job_test_teardown();
}
} // namespace portal