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

Job<int> c()
{
    LOG_INFO("C");
    co_return 1;
}

int main()
{
    Log::init({.default_log_level = Log::LogLevel::Trace});

    jobs::Scheduler scheduler{1};

    std::vector<Job<int>> a;
    a.emplace_back(c());
    a.emplace_back(c());

    [[maybe_unused]] auto result = scheduler.wait_for_jobs(std::span{a});


    LOG_INFO("COMPLETE");

    // ApplicationSpecification spec;
    // Application app{spec};
    // app.run();
}
