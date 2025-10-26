//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <nlohmann/json.hpp>

#include "portal/core/log.h"
#include "portal/core/jobs/job.h"
#include "portal/core/jobs/scheduler.h"
#include "portal/engine/application/application.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/vulkan/vulkan_window.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/folder_resource_database.h"

using namespace portal;

template <class Rep, class Period>
void simulate_work(const std::chrono::duration<Rep, Period>& duration)
{
    [[maybe_unused]] volatile double sink = 0.0;

    const auto start = std::chrono::high_resolution_clock::now();
    int i = 1;
    while (std::chrono::high_resolution_clock::now() - start < duration)
    {
        // Simulate some work
        sink = std::sqrt(++i);
    }
}

Job<std::thread::id> get_thread_id()
{
    auto thread_id = std::this_thread::get_id();
    simulate_work(std::chrono::microseconds(10));
    co_await SuspendJob();
    co_return thread_id;
}

int main()
{
    Log::init({.default_log_level = Log::LogLevel::Trace});

    jobs::Scheduler scheduler{2};

    std::vector<Job<std::thread::id>> jobs;
    for (int i = 0; i < 1000; ++i)
    {
        jobs.push_back(get_thread_id());
    }

    auto thread_ids = scheduler.wait_for_jobs(std::span{jobs.begin(), jobs.end()});

    std::unordered_map<std::thread::id, size_t> thread_appearances;
    for (auto& tid: thread_ids)
        thread_appearances[tid]++;

    for (auto& [tid, count] : thread_appearances)
        LOG_INFO("Thread {} ran {} jobs ", std::format("{}", tid), count);

    LOG_INFO("COMPLETE");

    auto& stats = scheduler.get_stats();
    stats.aggregate();
    stats.log();

    // ApplicationSpecification spec;
    // Application app{spec};
    // app.run();
}
