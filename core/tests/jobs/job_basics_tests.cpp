//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_test_macros.hpp>
#include <portal/core/jobs/job.h>
#include "portal/core/log.h"
#include "portal/core/jobs/scheduler.h"
#include <memory>

#include "common.h"

namespace portal
{
// ============================================================================
// Section 1: Job Basics
// ============================================================================

// ============================================================================
// Section 1.1: Single Job Execution Tests
// ============================================================================

Job<> simple_job(bool& executed)
{
    executed = true;
    co_await SuspendJob();
    co_return;
}

Job<> void_return_job(int& counter)
{
    ++counter;
    co_return;
}

Job<> job_for_lifecycle_test()
{
    co_return;
}

TEST_CASE("Single Job Execution", "[jobs][basics]")
{
    job_test_setup();

    SECTION("SingleJobCompletes")
    {
        jobs::Scheduler scheduler{0};
        bool executed = false;

        scheduler.wait_for_job(simple_job(executed));

        REQUIRE(executed);
    }

    SECTION("JobReturnsVoidProperly")
    {
        jobs::Scheduler scheduler{0};
        int counter = 0;

        scheduler.wait_for_job(void_return_job(counter));

        REQUIRE(counter == 1);
    }

    SECTION("JobCanBeCreatedAndDestroyedWithoutExecution")
    {
        // Create a job but don't dispatch it - should destroy cleanly
        Job<> job = job_for_lifecycle_test();

        // Job goes out of scope here without being dispatched
        // Should not leak or crash
    }

    job_test_teardown();
}

// ============================================================================
// Section 1.2: Job Return Values Tests
// ============================================================================

Job<int> job_returns_int(int value)
{
    co_return value;
}

Job<std::string> job_returns_string(std::string value)
{
    co_return value;
}

TEST_CASE("Job Return Values", "[jobs][basics]")
{
    job_test_setup();

    SECTION("JobReturnsValue")
    {
        jobs::Scheduler scheduler{0};

        auto result = scheduler.wait_for_job(job_returns_int(42));

        REQUIRE(result == 42);
    }

    SECTION("JobReturnsMultipleValues")
    {
        jobs::Scheduler scheduler{0};

        std::vector<Job<int>> jobs;
        for (int i = 0; i < 5; ++i)
        {
            jobs.push_back(job_returns_int(i * 10));
        }

        auto results = scheduler.wait_for_jobs(std::span{jobs.begin(), jobs.end()});

        REQUIRE(results.size() == 5);
        for (int i = 0; i < 5; ++i)
        {
            REQUIRE(results[i] == i * 10);
        }
    }

    SECTION("ResultReturnsStdExpectedWithValue")
    {
        jobs::Scheduler scheduler{0};

        auto job = job_returns_int(123);
        scheduler.wait_for_job(std::move(job));

        // Create another job to test result()
        auto job2 = job_returns_int(456);
        std::vector<Job<int>> jobs;
        jobs.push_back(std::move(job2));

        scheduler.wait_for_jobs(std::span{jobs});

        auto result = jobs[0].result();
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 456);
    }

    SECTION("ResultReturnsErrorWhenJobNotDispatched")
    {
        auto job = job_returns_int(789);

        // Call result() without dispatching the job
        auto result = job.result();

        // The job was never dispatched/executed, so result() should return an error
        REQUIRE_FALSE(result.has_value());
        if (!result.has_value())
        {
            REQUIRE(result.error() == JobResultStatus::Missing);
        }
    }

    SECTION("ResultOnVoidJobReturnsVoidTypeError")
    {
        jobs::Scheduler scheduler{0};
        int counter = 0;
        auto job = void_return_job(counter);

        scheduler.wait_for_job(std::move(job));

        // Calling result() on Job<void> should compile but return VoidType error
        auto result = job.result();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == JobResultStatus::VoidType);
    }

    SECTION("MoveOnlyReturnTypes")
    {
        // Move-only type for testing
        struct MoveOnlyType
        {
            int value = 0;

            MoveOnlyType() = default;
            explicit MoveOnlyType(int v) : value(v) {}

            MoveOnlyType(const MoveOnlyType&) = delete;
            MoveOnlyType& operator=(const MoveOnlyType&) = delete;

            MoveOnlyType(MoveOnlyType&&) noexcept = default;
            MoveOnlyType& operator=(MoveOnlyType&&) noexcept = default;

            static Job<MoveOnlyType> job_returns_move_only(int value)
            {
                co_return MoveOnlyType{value};
            }
        };

        jobs::Scheduler scheduler{0};

        auto result = scheduler.wait_for_job(MoveOnlyType::job_returns_move_only(999));

        REQUIRE(result.value == 999);
    }

    job_test_teardown();
}

// ============================================================================
// Section 1.3: SuspendJob Awaitable Tests
// ============================================================================

Job<> job_with_suspension(int& counter)
{
    counter = 1;
    co_await SuspendJob();
    counter = 2;
    co_return;
}

Job<> job_with_multiple_suspensions(std::vector<int>& execution_order)
{
    execution_order.push_back(1);
    co_await SuspendJob();

    execution_order.push_back(2);
    co_await SuspendJob();

    execution_order.push_back(3);
    co_await SuspendJob();

    execution_order.push_back(4);
    co_return;
}

Job<int> job_with_suspension_at_different_points(int suspend_point)
{
    int result = 0;

    if (suspend_point == 1)
        co_await SuspendJob();

    result += 10;

    if (suspend_point == 2)
        co_await SuspendJob();

    result += 20;

    if (suspend_point == 3)
        co_await SuspendJob();

    result += 30;

    co_return result;
}

TEST_CASE("SuspendJob Awaitable", "[jobs][basics]")
{
    job_test_setup();

    SECTION("JobSuspendsAndResumesCorrectly")
    {
        jobs::Scheduler scheduler{0};
        int counter = 0;

        scheduler.wait_for_job(job_with_suspension(counter));

        REQUIRE(counter == 2);
    }

    SECTION("MultipleSuspensionsInSingleJob")
    {
        jobs::Scheduler scheduler{0};
        std::vector<int> execution_order;

        scheduler.wait_for_job(job_with_multiple_suspensions(execution_order));

        REQUIRE(execution_order.size() == 4);
        REQUIRE(execution_order[0] == 1);
        REQUIRE(execution_order[1] == 2);
        REQUIRE(execution_order[2] == 3);
        REQUIRE(execution_order[3] == 4);
    }

    SECTION("SuspensionAtDifferentPointsInJobExecution")
    {
        jobs::Scheduler scheduler{0};

        auto result1 = scheduler.wait_for_job(job_with_suspension_at_different_points(1));
        REQUIRE(result1 == 60);

        auto result2 = scheduler.wait_for_job(job_with_suspension_at_different_points(2));
        REQUIRE(result2 == 60);

        auto result3 = scheduler.wait_for_job(job_with_suspension_at_different_points(3));
        REQUIRE(result3 == 60);
    }

    job_test_teardown();
}

// ============================================================================
// Section 1.4: FinalizeJob Tests
// ============================================================================

TEST_CASE("FinalizeJob", "[jobs][basics]")
{
    job_test_setup();

    SECTION("FinalSuspendDecrementsCounterCount")
    {
        jobs::Scheduler scheduler{0};
        jobs::Counter counter{};

        std::vector<uint8_t> flags(3, false);
        std::vector<Job<>> jobs;
        for (auto& val : flags)
            jobs.push_back(simple_job(reinterpret_cast<bool&>(val)));

        // Dispatch with counter
        scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal, &counter);

        // Counter should be incremented to 3
        REQUIRE(counter.count.load() == 3);

        // Wait for jobs to complete
        while (counter.count.load(std::memory_order_acquire) > 0)
        {
            scheduler.main_thread_do_work();
        }

        // Counter should be back to 0
        REQUIRE(counter.count.load() == 0);
    }

    SECTION("CounterUnblocksWhenLastJobFinalizes")
    {
        jobs::Scheduler scheduler{0};

        std::vector<Job<>> jobs;
        bool executed = false;
        jobs.push_back(simple_job(executed));

        // wait_for_jobs should block until job completes and counter reaches 0
        scheduler.wait_for_jobs(std::span{jobs});

        REQUIRE(executed);
    }

    job_test_teardown();
}

// ============================================================================
// Section 1.5: Job Lifecycle Tests
// ============================================================================

TEST_CASE("Job Lifecycle", "[jobs][basics]")
{
    job_test_setup();

    SECTION("JobBaseMoveConstructorTransfersHandle")
    {
        auto job1 = job_for_lifecycle_test();
        auto original_handle = job1.handle;

        // Move construct
        auto job2 = std::move(job1);

        REQUIRE(job2.handle == original_handle);
        REQUIRE(job1.handle == nullptr);
    }

    SECTION("JobBaseMoveAssignmentTransfersHandle")
    {
        auto job1 = job_for_lifecycle_test();
        auto job2 = job_for_lifecycle_test();

        auto original_handle = job1.handle;

        // Move assign
        job2 = std::move(job1);

        REQUIRE(job2.handle == original_handle);
        REQUIRE(job1.handle == nullptr);
    }

    SECTION("DestructorDestroysHandleOnlyIfNotDispatched")
    {
        jobs::Scheduler scheduler{0};

        {
            auto job = job_for_lifecycle_test();
            // Job goes out of scope without being dispatched
            // Destructor should destroy the handle
        }

        {
            bool executed = false;
            auto job = simple_job(executed);
            scheduler.wait_for_job(std::move(job));
            // Job was dispatched, destructor should NOT destroy handle
            // (it was already destroyed by the scheduler)
        }
    }

    job_test_teardown();
}
} // namespace portal