//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_test_macros.hpp>
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
        tracker->record(fmt::format("inner_{}_{}", i, j));
    }

    co_await SuspendJob();
    co_return;
}

Job<> outer_coroutine(int i, jobs::Scheduler& scheduler, ExecutionTracker* tracker = nullptr)
{
    if (tracker)
    {
        tracker->record(fmt::format("outer_{}", i));
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

TEST_CASE("Nested Jobs Basic", "[jobs][nested]")
{
    job_test_setup();

    SECTION("JobWithinJob")
    {
        jobs::Scheduler scheduler{1};
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
            REQUIRE(tracker.was_executed(fmt::format("outer_{}", i)));
        }

        // Verify all inner coroutines executed (20 outer * 5 inner each = 100)
        for (int i = 0; i < 20; ++i)
        {
            for (int j = 0; j < 5; ++j)
            {
                REQUIRE(tracker.was_executed(fmt::format("inner_{}_{}", i, j)));
            }
        }

        // Verify total execution count (20 outer + 100 inner = 120)
        REQUIRE(tracker.execution_count() == 120);

        // Verify execution order: each outer coroutine should execute before its inner coroutines
        for (int i = 0; i < 20; ++i)
        {
            for (int j = 0; j < 5; ++j)
            {
                REQUIRE(
                    tracker.executed_before(
                        fmt::format("outer_{}", i),
                        fmt::format("inner_{}_{}", i, j))
                );
            }
        }
    }

    job_test_teardown();
}

// ============================================================================
// Section 5.2: Deep Nesting
// ============================================================================

Job<> level3_job(int i, int j, int k, ExecutionTracker* tracker)
{
    tracker->record(fmt::format("level3_{}_{}_{}", i, j, k));
    co_await SuspendJob();
    co_return;
}

Job<> level2_job(int i, int j, jobs::Scheduler& scheduler, ExecutionTracker* tracker)
{
    tracker->record(fmt::format("level2_{}_{}", i, j));

    std::vector<Job<>> jobs;
    for (int k = 0; k < 3; ++k)
    {
        jobs.push_back(level3_job(i, j, k, tracker));
    }

    co_await SuspendJob();
    scheduler.wait_for_jobs(std::span{jobs});
    co_return;
}

Job<> level1_job(int i, jobs::Scheduler& scheduler, ExecutionTracker* tracker)
{
    tracker->record(fmt::format("level1_{}", i));

    std::vector<Job<>> jobs;
    for (int j = 0; j < 2; ++j)
    {
        jobs.push_back(level2_job(i, j, scheduler, tracker));
    }

    co_await SuspendJob();
    scheduler.wait_for_jobs(std::span{jobs});
    co_return;
}

TEST_CASE("Deep Nesting", "[jobs][nested]")
{
    job_test_setup();

    SECTION("ThreeLevelsOfNestedJobs")
    {
        jobs::Scheduler scheduler{1};
        ExecutionTracker tracker;

        std::vector<Job<>> jobs;
        for (int i = 0; i < 2; ++i)
        {
            jobs.push_back(level1_job(i, scheduler, &tracker));
        }

        scheduler.wait_for_jobs(std::span{jobs});

        // Verify all level1 jobs executed (2)
        for (int i = 0; i < 2; ++i)
        {
            REQUIRE(tracker.was_executed(fmt::format("level1_{}", i)));
        }

        // Verify all level2 jobs executed (2 * 2 = 4)
        for (int i = 0; i < 2; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                REQUIRE(tracker.was_executed(fmt::format("level2_{}_{}", i, j)));
            }
        }

        // Verify all level3 jobs executed (2 * 2 * 3 = 12)
        for (int i = 0; i < 2; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                for (int k = 0; k < 3; ++k)
                {
                    REQUIRE(tracker.was_executed(fmt::format("level3_{}_{}_{}", i, j, k)));
                }
            }
        }

        // Total: 2 + 4 + 12 = 18
        REQUIRE(tracker.execution_count() == 18);
    }

    job_test_teardown();
}

Job<> multilevel_spawner(int level, int max_level, jobs::Scheduler& scheduler, ExecutionTracker* tracker)
{
    tracker->record(fmt::format("level_{}", level));

    if (level < max_level)
    {
        std::vector<Job<>> jobs;
        for (int i = 0; i < 3; ++i)
        {
            jobs.push_back(multilevel_spawner(level + 1, max_level, scheduler, tracker));
        }

        co_await SuspendJob();
        scheduler.wait_for_jobs(std::span{jobs});
    }

    co_return;
}

TEST_CASE("Multilevel Job Spawning", "[jobs][nested]")
{
    job_test_setup();

    SECTION("EachLevelSpawnsMultipleSubJobs")
    {
        jobs::Scheduler scheduler{2};
        ExecutionTracker tracker;

        // 3 levels: level 0 -> 3x level 1 -> 9x level 2
        scheduler.wait_for_job(multilevel_spawner(0, 2, scheduler, &tracker));

        // Level 0: 1 job
        // Level 1: 3 jobs
        // Level 2: 9 jobs
        // Total: 13 jobs
        REQUIRE(tracker.execution_count() == 13);

        REQUIRE(tracker.was_executed("level_0"));

        // Verify level 0 executed before all level 1 jobs
        for (int i = 0; i < 3; ++i)
        {
            REQUIRE(tracker.was_executed("level_1"));
        }

        // Verify level 1 jobs exist and level 2 jobs exist
        for (int i = 0; i < 9; ++i)
        {
            REQUIRE(tracker.was_executed("level_2"));
        }
    }

    SECTION("VerifyExecutionOrderAtAllLevels")
    {
        jobs::Scheduler scheduler{1};
        ExecutionTracker tracker;

        scheduler.wait_for_job(level1_job(0, scheduler, &tracker));

        // Verify level1_0 executes before all level2 jobs
        REQUIRE(tracker.executed_before("level1_0", "level2_0_0"));
        REQUIRE(tracker.executed_before("level1_0", "level2_0_1"));

        // Verify level2 jobs execute before their level3 children
        REQUIRE(tracker.executed_before("level2_0_0", "level3_0_0_0"));
        REQUIRE(tracker.executed_before("level2_0_0", "level3_0_0_1"));
        REQUIRE(tracker.executed_before("level2_0_0", "level3_0_0_2"));

        REQUIRE(tracker.executed_before("level2_0_1", "level3_0_1_0"));
        REQUIRE(tracker.executed_before("level2_0_1", "level3_0_1_1"));
        REQUIRE(tracker.executed_before("level2_0_1", "level3_0_1_2"));
    }

    job_test_teardown();
}

// ============================================================================
// Section 5.3: Nested Job Edge Cases
// ============================================================================

Job<> outer_with_no_inner(ExecutionTracker* tracker)
{
    tracker->record("outer_no_inner");
    co_await SuspendJob();

    // Don't spawn any inner jobs
    std::vector<Job<>> empty_jobs;

    co_return;
}

TEST_CASE("Nested Job Edge Cases", "[jobs][nested]")
{
    job_test_setup();

    SECTION("OuterJobWithNoInnerJobs")
    {
        jobs::Scheduler scheduler{0};
        ExecutionTracker tracker;

        scheduler.wait_for_job(outer_with_no_inner(&tracker));

        REQUIRE(tracker.was_executed("outer_no_inner"));
        REQUIRE(tracker.execution_count() == 1);
    }

    job_test_teardown();
}

Job<> recursive_spawner(int depth, jobs::Scheduler& scheduler, ExecutionTracker* tracker)
{
    tracker->record(fmt::format("recursive_{}", depth));

    if (depth > 0)
    {
        // Spawn one sub-job that spawns its own sub-jobs
        auto sub_job = recursive_spawner(depth - 1, scheduler, tracker);
        std::vector<Job<>> jobs;
        jobs.push_back(std::move(sub_job));

        co_await SuspendJob();
        scheduler.wait_for_jobs(std::span{jobs});
    }

    co_return;
}

TEST_CASE("Recursive Job Spawning", "[jobs][nested]")
{
    job_test_setup();

    SECTION("InnerJobSpawnsItsOwnSubJobs")
    {
        jobs::Scheduler scheduler{1};
        ExecutionTracker tracker;

        scheduler.wait_for_job(recursive_spawner(3, scheduler, &tracker));

        // Should have recursive_3, recursive_2, recursive_1, recursive_0
        REQUIRE(tracker.execution_count() == 4);

        REQUIRE(tracker.was_executed("recursive_3"));
        REQUIRE(tracker.was_executed("recursive_2"));
        REQUIRE(tracker.was_executed("recursive_1"));
        REQUIRE(tracker.was_executed("recursive_0"));

        // Verify execution order
        REQUIRE(tracker.executed_before("recursive_3", "recursive_2"));
        REQUIRE(tracker.executed_before("recursive_2", "recursive_1"));
        REQUIRE(tracker.executed_before("recursive_1", "recursive_0"));
    }

    job_test_teardown();
}

Job<> nested_with_suspension_level3(int id, ExecutionTracker* tracker)
{
    tracker->record(fmt::format("suspend_l3_{}_before", id));
    co_await SuspendJob();
    tracker->record(fmt::format("suspend_l3_{}_after", id));
    co_return;
}

Job<> nested_with_suspension_level2(int id, jobs::Scheduler& scheduler, ExecutionTracker* tracker)
{
    tracker->record(fmt::format("suspend_l2_{}_before", id));
    co_await SuspendJob();
    tracker->record(fmt::format("suspend_l2_{}_middle", id));

    std::vector<Job<>> jobs;
    jobs.push_back(nested_with_suspension_level3(id, tracker));
    scheduler.wait_for_jobs(std::span{jobs});

    tracker->record(fmt::format("suspend_l2_{}_after", id));
    co_return;
}

Job<> nested_with_suspension_level1(jobs::Scheduler& scheduler, ExecutionTracker* tracker)
{
    tracker->record("suspend_l1_before");
    co_await SuspendJob();
    tracker->record("suspend_l1_middle");

    std::vector<Job<>> jobs;
    jobs.push_back(nested_with_suspension_level2(0, scheduler, tracker));
    scheduler.wait_for_jobs(std::span{jobs});

    tracker->record("suspend_l1_after");
    co_return;
}

TEST_CASE("Nested Jobs with Suspensions", "[jobs][nested]")
{
    job_test_setup();

    SECTION("NestedJobsWithSuspensionsAtEachLevel")
    {
        jobs::Scheduler scheduler{1};
        ExecutionTracker tracker;

        scheduler.wait_for_job(nested_with_suspension_level1(scheduler, &tracker));

        // Verify all suspension points executed
        REQUIRE(tracker.was_executed("suspend_l1_before"));
        REQUIRE(tracker.was_executed("suspend_l1_middle"));
        REQUIRE(tracker.was_executed("suspend_l1_after"));

        REQUIRE(tracker.was_executed("suspend_l2_0_before"));
        REQUIRE(tracker.was_executed("suspend_l2_0_middle"));
        REQUIRE(tracker.was_executed("suspend_l2_0_after"));

        REQUIRE(tracker.was_executed("suspend_l3_0_before"));
        REQUIRE(tracker.was_executed("suspend_l3_0_after"));

        // Total: 3 + 3 + 2 = 8 execution points
        REQUIRE(tracker.execution_count() == 8);

        // Verify execution order respects suspension and nesting
        REQUIRE(tracker.executed_before("suspend_l1_before", "suspend_l1_middle"));
        REQUIRE(tracker.executed_before("suspend_l1_middle", "suspend_l2_0_before"));
        REQUIRE(tracker.executed_before("suspend_l2_0_before", "suspend_l2_0_middle"));
        REQUIRE(tracker.executed_before("suspend_l2_0_middle", "suspend_l3_0_before"));
        REQUIRE(tracker.executed_before("suspend_l3_0_before", "suspend_l3_0_after"));
        REQUIRE(tracker.executed_before("suspend_l3_0_after", "suspend_l2_0_after"));
        REQUIRE(tracker.executed_before("suspend_l2_0_after", "suspend_l1_after"));
    }

    job_test_teardown();
}
} // namespace portal