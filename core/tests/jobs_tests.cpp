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

namespace portal
{

// Thread-safe execution tracker for testing
struct ExecutionTracker
{
    std::mutex mutex;
    std::vector<std::string> execution_order;
    std::set<std::string> executed_coroutines;

    void record(const std::string& coroutine_id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        execution_order.push_back(coroutine_id);
        executed_coroutines.insert(coroutine_id);
    }

    bool was_executed(const std::string& coroutine_id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return executed_coroutines.contains(coroutine_id);
    }

    size_t execution_count()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return execution_order.size();
    }

    // Check if coroutine A executed before coroutine B
    bool executed_before(const std::string& a, const std::string& b)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it_a = std::find(execution_order.begin(), execution_order.end(), a);
        auto it_b = std::find(execution_order.begin(), execution_order.end(), b);
        return it_a != execution_order.end() && it_b != execution_order.end() && it_a < it_b;
    }
};

// ============================================================================
// Section 1.1: Single Job Execution Tests
// ============================================================================

Job<> simple_job(bool& executed)
{
    executed = true;
    co_return;
}

TEST(JobTest, SingleJobCompletes)
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

TEST(JobTest, JobReturnsVoidProperly)
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

// ============================================================================
// Section 6.1: Multi-Threading Tests
// ============================================================================

Job<int> get_thread_id(int i)
{
    co_return i;
}

TEST(JobTest, MultiThreadedExecution)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(1);

    std::vector<Job<int>> jobs{};
    for (int i = 0; i < 5; ++i)
    {
        jobs.emplace_back(get_thread_id(i));
    }

    scheduler.wait_for_jobs<int>(jobs);

    // Validate that there are two distinct thread ids
    std::set<std::thread::id> distinct_thread_ids;

    EXPECT_EQ(distinct_thread_ids.size(), 2);
}

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


    std::vector<Job<>> jobs{};
    for (size_t j = 0; j < 5; ++j)
    {
        jobs.push_back(inner_coroutine(i, j, tracker));
    }


    co_await SuspendJob();
    scheduler.wait_for_jobs<void>(jobs);

    co_return;
}

TEST(JobTest, JobWithinJob)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(1);
    ExecutionTracker tracker;

    std::vector<Job<>> jobs{};
    for (int i = 0; i < 20; ++i)
    {
        jobs.push_back(outer_coroutine(i, scheduler, &tracker));
    }

    scheduler.wait_for_jobs<void>(jobs);


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

}
