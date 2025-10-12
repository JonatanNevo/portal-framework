//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <nlohmann/json.hpp>

#include "portal/core/log.h"
#include "portal/core/jobs/job.h"
#include "portal/core/jobs/job_list.h"
#include "portal/core/jobs/scheduler.h"
#include "portal/engine/application/application.h"
#include "portal/engine/application/window.h"
#include "portal/engine/events/window_events.h"
#include "portal/engine/imgui/im_gui_module.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/vulkan/vulkan_window.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/folder_resource_database.h"

using namespace portal;



Job do_something(int i)
{
    LOG_INFO("Doing something: {}", i);
    co_await SuspendJob();

    LOG_INFO("Resuming something: {}", i);
    co_return;
}


int main()
{
    Log::init({.default_log_level = Log::LogLevel::Trace});

    jobs::Scheduler scheduler = jobs::Scheduler::create(1);

    JobList jobs{};
    for (int i = 0; i < 5; ++i)
    {
        jobs.add_job(do_something(i));
    }
    scheduler.wait_for_job_list(jobs);

    LOG_INFO("COMPLETE");

    //
    //
    // ApplicationSpecification spec;
    // Application app{spec};
    // app.run();
}
