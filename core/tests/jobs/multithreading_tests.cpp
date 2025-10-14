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
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    co_await SuspendJob();
    co_return thread_id;
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

    size_t index = 0;
    for (auto& job : jobs)
    {
        auto& promise = job.handle.promise();
        for (auto& [id, time, type] : promise.switch_information)
        {
            std::string thread_id = std::format("{}", id);
            std::string type_string;
            switch (type)
            {
            case SwitchType::Start:
                type_string = "Start";
                break;
            case SwitchType::Resume:
                type_string = "Resume";
                break;
            case SwitchType::Pause:
                type_string = "Pause";
                break;
            case SwitchType::Finish:
                type_string = "Finish";
                break;
            case SwitchType::Error:
                type_string = "Error";
                break;
            }

            LOG_INFO("Job {} [{}] switched to {} at {}", index, thread_id, type_string, time);
        }
        index++;
    }

    std::unordered_map<std::thread::id, size_t> thread_appearances;

    for (auto& tid: thread_ids)
        thread_appearances[tid]++;

    for (auto& [tid, count] : thread_appearances)
        LOG_INFO("Thread {} ran {} jobs ", std::format("{}", tid), count);

    // With 1 worker thread + main thread, we should see 2 distinct thread IDs
    EXPECT_EQ(thread_appearances.size(), 2) << "Expected 2 distinct threads (main + 1 worker)";
}

} // namespace portal