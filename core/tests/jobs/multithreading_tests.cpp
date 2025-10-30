//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <portal/core/jobs/job.h>
#include "portal/core/log.h"
#include "portal/core/jobs/scheduler.h"
#include <set>
#include <fmt/chrono.h>

#include "common.h"

namespace portal
{

// ============================================================================
// Section 6: Multi-Threading Tests
// ============================================================================

Job<std::thread::id> get_thread_id()
{
    auto thread_id = std::this_thread::get_id();
    simulate_work(std::chrono::nanoseconds(10));
    co_await SuspendJob();
    co_return thread_id;
}

// ============================================================================
// Section 6.1: Thread Distribution
// ============================================================================

TEST_F(JobTest, MultiThreadedExecution)
{
    jobs::Scheduler scheduler{2};

    std::vector<Job<std::thread::id>> jobs;
    for (int i = 0; i < 1000; ++i)
    {
        jobs.push_back(get_thread_id());
    }

    auto thread_ids = scheduler.wait_for_jobs(std::span{jobs.begin(), jobs.end()});

    std::unordered_map<std::thread::id, size_t> thread_appearances;
    for (auto& tid : thread_ids)
        thread_appearances[tid]++;

    for (auto& [tid, count] : thread_appearances)
        LOG_INFO("Thread {} ran {} jobs ", std::format("{}", tid), count);

    // With 2 worker threads + main thread, we should see 3 distinct thread IDs
    EXPECT_EQ(thread_appearances.size(), 3) << "Expected 3 distinct threads (main + 2 workers)";
}

TEST_F(JobTest, LoadBalancingAcrossWorkerThreads)
{
    jobs::Scheduler scheduler{4};

    // Create enough jobs to ensure distribution across threads
    std::vector<Job<std::thread::id>> jobs;
    for (int i = 0; i < 1000; ++i)
    {
        jobs.push_back(get_thread_id());
    }

    auto thread_ids = scheduler.wait_for_jobs(std::span{jobs.begin(), jobs.end()});

    // Count jobs per thread
    std::unordered_map<std::thread::id, size_t> jobs_per_thread;
    for (const auto& tid : thread_ids)
    {
        jobs_per_thread[tid]++;
    }

    LOG_INFO("Load balancing results:");
    for (const auto& [tid, count] : jobs_per_thread)
    {
        LOG_INFO(
            "  Thread {} executed {} jobs ({:.1f}%)",
            std::format("{}", tid),
            count,
            (count * 100.0) / jobs.size()
            );
    }

    // Verify multiple threads participated
    EXPECT_GE(jobs_per_thread.size(), 3)
        << "Expected at least 3 threads to participate in execution";

    // Verify no single thread dominated execution (no thread should have > 80% of jobs)
    size_t max_jobs_on_single_thread = 0;
    for (const auto& [tid, count] : jobs_per_thread)
    {
        max_jobs_on_single_thread = std::max(max_jobs_on_single_thread, count);
    }

    double max_thread_percentage = (max_jobs_on_single_thread * 100.0) / jobs.size();
    EXPECT_LT(max_thread_percentage, 80.0)
        << "One thread executed " << max_thread_percentage
        << "% of jobs, indicating poor load balancing";

    // Verify reasonable distribution: each participating thread should have at least 5% of jobs
    for (const auto& [tid, count] : jobs_per_thread)
    {
        double thread_percentage = (count * 100.0) / jobs.size();
        EXPECT_GT(thread_percentage, 5.0)
            << "Thread executed only " << thread_percentage
            << "% of jobs, indicating uneven distribution";
    }
}


// ============================================================================
// Section 6.4: Work Stealing
// ============================================================================

Job<bool> make_work_for_steal(size_t i, jobs::Scheduler& scheduler, std::atomic<bool>& flag)
{
    static bool created = false;

    if (jobs::Scheduler::get_tls_worker_id() == 0)
    {
        std::vector<Job<bool>> jobs;

        if (!created)
        {
            for (int j = 1; j < 128; ++j)
            {
                jobs.push_back(make_work_for_steal(j, scheduler, flag));
            }
            scheduler.dispatch_jobs(std::span{jobs}, JobPriority::Normal);
            created = true;
        }

        // Wait for some other thread to reach here
        while (flag.load() == false)
        {
            co_await SuspendJob();
        }

        co_return true;
    }

    simulate_work(std::chrono::milliseconds{10});

    // If some thread reached here, mark all threads for finish (someone stole)
    if (created)
        flag.store(true);
    co_return false;
}

// TODO: this does not work
// TEST_F(JobTest, JobStealing)
// {
//     jobs::Scheduler scheduler{3, 1};
//
//     std::atomic<bool> flag;
//     std::vector<Job<bool>> jobs;
//
//     // No worker affinity on job dispatch so make multiple root level jobs
//     jobs.push_back(make_work_for_steal(0, scheduler, flag));
//     jobs.push_back(make_work_for_steal(0, scheduler, flag));
//     jobs.push_back(make_work_for_steal(0, scheduler, flag));
//
//     auto res = scheduler.wait_for_jobs(std::span{jobs});
//     EXPECT_TRUE(std::ranges::any_of(res, [](bool b) { return b; }));
// }

Job<> run_on_specific_worker(size_t i, size_t worker_id)
{
    auto curr_thread = jobs::Scheduler::get_tls_worker_id();
    while (curr_thread == worker_id)
    {
        co_await SuspendJob();
        curr_thread = jobs::Scheduler::get_tls_worker_id();
    }

    LOG_INFO("Reached other thread: {}", i);
    co_return;
}

Job<bool> counter_deadlock(size_t i, jobs::Scheduler& scheduler)
{
    LOG_INFO("Starting: {}", i);

    auto curr_thread = jobs::Scheduler::get_tls_worker_id();
    while (curr_thread != 0)
    {
        co_await SuspendJob();
        curr_thread = jobs::Scheduler::get_tls_worker_id();
    }

    LOG_INFO("Found worker thread: {}", i);
    std::vector<Job<>> jobs;
    for (int j = 1; j < 128; ++j)
    {
        jobs.push_back(run_on_specific_worker(j, 0));
    }
    LOG_INFO("Dispatching jobs: {}", i);
    scheduler.wait_for_jobs(std::span{jobs});
    co_return true;
}

// TODO: this does not work
// TEST_F(JobTest, CounterDeadlockBug)
// {
//
//     jobs::Scheduler scheduler{1, 1};
//     std::vector<Job<bool>> jobs;
//     // No worker affinity on job dispatch so make multiple root level jobs
//     jobs.push_back(counter_deadlock(0, scheduler));
//
//     auto res = scheduler.wait_for_jobs(std::span{jobs});
// }
} // namespace portal
