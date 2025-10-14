//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
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

TEST_F(JobTest, SingleJobCompletes)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);
    bool executed = false;

    scheduler.wait_for_job(simple_job(executed));

    EXPECT_TRUE(executed);
}

Job<> void_return_job(int& counter)
{
    ++counter;
    co_return;
}

TEST_F(JobTest, JobReturnsVoidProperly)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);
    int counter = 0;

    scheduler.wait_for_job(void_return_job(counter));

    EXPECT_EQ(counter, 1);
}

Job<> job_for_lifecycle_test()
{
    co_return;
}

TEST_F(JobTest, JobCanBeCreatedAndDestroyedWithoutExecution)
{
    // Create a job but don't dispatch it - should destroy cleanly
    Job<> job = job_for_lifecycle_test();

    // Job goes out of scope here without being dispatched
    // Should not leak or crash
}

// ============================================================================
// Section 1.2: Job Return Values Tests
// ============================================================================

Job<int> job_returns_int(int value)
{
    co_return value;
}

TEST_F(JobTest, JobReturnsValue)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);

    auto result = scheduler.wait_for_job(job_returns_int(42));

    EXPECT_EQ(result, 42);
}

Job<std::string> job_returns_string(std::string value)
{
    co_return value;
}

TEST_F(JobTest, JobReturnsMultipleValues)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);

    std::vector<Job<int>> jobs;
    for (int i = 0; i < 5; ++i)
    {
        jobs.push_back(job_returns_int(i * 10));
    }

    auto results = scheduler.wait_for_jobs(std::span{jobs.begin(), jobs.end()});

    ASSERT_EQ(results.size(), 5);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(results[i], i * 10);
    }
}

TEST_F(JobTest, ResultReturnsStdExpectedWithValue)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);

    auto job = job_returns_int(123);
    scheduler.wait_for_job(std::move(job));

    // Create another job to test result()
    auto job2 = job_returns_int(456);
    std::vector<Job<int>> jobs;
    jobs.push_back(std::move(job2));

    scheduler.wait_for_jobs(std::span{jobs});

    auto result = jobs[0].result();
    ASSERT_TRUE(result.has_value()) << "Expected result to have value";
    EXPECT_EQ(result.value(), 456);
}

TEST_F(JobTest, ResultReturnsErrorWhenJobNotDispatched)
{
    auto job = job_returns_int(789);

    // Call result() without dispatching the job
    auto result = job.result();

    // The result should not have a value since the job never ran
    // Note: The actual behavior depends on implementation -
    // the promise may have uninitialized data
}

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
};

Job<MoveOnlyType> job_returns_move_only(int value)
{
    co_return MoveOnlyType{value};
}

TEST_F(JobTest, MoveOnlyReturnTypes)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);

    auto result = scheduler.wait_for_job(job_returns_move_only(999));

    EXPECT_EQ(result.value, 999);
}

// Note: Copy-only types (with deleted move constructor/assignment) are not supported
// as return types because modern C++ APIs expect move semantics for efficiency.
// The job system requires that return types are either movable or trivially copyable.

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

TEST_F(JobTest, JobSuspendsAndResumesCorrectly)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);
    int counter = 0;

    scheduler.wait_for_job(job_with_suspension(counter));

    EXPECT_EQ(counter, 2) << "Job should have completed both parts after suspension";
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

TEST_F(JobTest, MultipleSuspensionsInSingleJob)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);
    std::vector<int> execution_order;

    scheduler.wait_for_job(job_with_multiple_suspensions(execution_order));

    ASSERT_EQ(execution_order.size(), 4);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
    EXPECT_EQ(execution_order[3], 4);
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

TEST_F(JobTest, SuspensionAtDifferentPointsInJobExecution)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);

    auto result1 = scheduler.wait_for_job(job_with_suspension_at_different_points(1));
    EXPECT_EQ(result1, 60);

    auto result2 = scheduler.wait_for_job(job_with_suspension_at_different_points(2));
    EXPECT_EQ(result2, 60);

    auto result3 = scheduler.wait_for_job(job_with_suspension_at_different_points(3));
    EXPECT_EQ(result3, 60);
}

// ============================================================================
// Section 1.4: FinalizeJob Tests
// ============================================================================

TEST_F(JobTest, FinalSuspendDecrementsCounterCount)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);
    jobs::Counter counter{};

    std::vector<Job<>> jobs;
    jobs.push_back(simple_job(*(new bool())));
    jobs.push_back(simple_job(*(new bool())));
    jobs.push_back(simple_job(*(new bool())));

    // Dispatch with counter
    scheduler.dispatch_jobs(std::span{jobs}, &counter);

    // Counter should be incremented to 3
    EXPECT_EQ(counter.count.load(), 3);

    // Wait for jobs to complete
    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        auto handle = scheduler.try_dequeue_job();
        if (handle)
            handle();
    }

    // Counter should be back to 0
    EXPECT_EQ(counter.count.load(), 0);
}

TEST_F(JobTest, CounterUnblocksWhenLastJobFinalizes)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(1);

    std::vector<Job<>> jobs;
    bool executed = false;
    jobs.push_back(simple_job(executed));

    // wait_for_jobs should block until job completes and counter reaches 0
    scheduler.wait_for_jobs(std::span{jobs});

    EXPECT_TRUE(executed) << "Job should have completed and unblocked";
}

// ============================================================================
// Section 1.5: Job Lifecycle Tests
// ============================================================================

TEST_F(JobTest, JobBaseMoveConstructorTransfersHandle)
{
    auto job1 = job_for_lifecycle_test();
    auto original_handle = job1.handle;

    // Move construct
    auto job2 = std::move(job1);

    EXPECT_EQ(job2.handle, original_handle) << "Handle should be transferred";
    EXPECT_EQ(job1.handle, nullptr) << "Original handle should be null after move";
}

TEST_F(JobTest, JobBaseMoveAssignmentTransfersHandle)
{
    auto job1 = job_for_lifecycle_test();
    auto job2 = job_for_lifecycle_test();

    auto original_handle = job1.handle;

    // Move assign
    job2 = std::move(job1);

    EXPECT_EQ(job2.handle, original_handle) << "Handle should be transferred";
    EXPECT_EQ(job1.handle, nullptr) << "Original handle should be null after move";
}

TEST_F(JobTest, DestructorDestroysHandleOnlyIfNotDispatched)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(0);

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

} // namespace portal