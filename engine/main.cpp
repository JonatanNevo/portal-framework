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
//
// Job<int> c()
// {
//     LOG_INFO("C");
//     co_return 1;
// }
//
// Job<std::thread::id> get_thread_id()
// {
//     auto thread_id = std::this_thread::get_id();
//     std::this_thread::sleep_for(std::chrono::microseconds(1));
//     co_await SuspendJob();
//     co_return thread_id;
// }

int main()
{
    // Log::init({.default_log_level = Log::LogLevel::Trace});
    //
    // jobs::Scheduler scheduler{1};
    //
    // std::vector<Job<std::thread::id>> jobs;
    // for (int i = 0; i < 100; ++i)
    // {
    //     jobs.push_back(get_thread_id());
    // }
    //
    // auto thread_ids = scheduler.wait_for_jobs(std::span{jobs.begin(), jobs.end()});
    //
    // std::unordered_map<std::thread::id, size_t> thread_appearances;
    // for (auto& tid: thread_ids)
    //     thread_appearances[tid]++;
    //
    // for (auto& [tid, count] : thread_appearances)
    //     LOG_INFO("Thread {} ran {} jobs ", std::format("{}", tid), count);
    //
    // LOG_INFO("COMPLETE");

    ApplicationSpecification spec;
    Application app{spec};
    app.run();
}
