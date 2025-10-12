//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <portal/core/jobs/job.h>
#include "portal/core/log.h"
#include "portal/core/jobs/scheduler.h"
#include <set>

#include "common.h"

namespace portal
{

// ============================================================================
// Section 6: Multi-Threading Tests
// ============================================================================

Job<std::thread::id> get_thread_id()
{
    co_await SuspendJob();
    co_return std::this_thread::get_id();
}

TEST_F(JobTest, MultiThreadedExecution)
{
    jobs::Scheduler scheduler = jobs::Scheduler::create(1);

    std::vector<Job<std::thread::id>> jobs;
    for (int i = 0; i < 10; ++i)
    {
        jobs.push_back(get_thread_id());
    }

    auto thread_ids = scheduler.wait_for_jobs(std::span{jobs.begin(), jobs.end()});

    // Collect distinct thread IDs
    std::set<std::thread::id> distinct_thread_ids(thread_ids.begin(), thread_ids.end());

    // With 1 worker thread + main thread, we should see 2 distinct thread IDs
    EXPECT_EQ(distinct_thread_ids.size(), 2) << "Expected 2 distinct threads (main + 1 worker)";
}

} // namespace portal