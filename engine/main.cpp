//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include "portal/core/files/file_system.h"
#include "portal/engine/settings.h"
#include "portal/engine/application/application.h"
#include "glaze/core/reflect.hpp"
#include "portal/input/new/keys.h"

using namespace portal;

constexpr auto LOG_LEVEL_ENTRY = "log-level";

void initialize_settings()
{
    Log::init({.default_log_level = Log::LogLevel::Trace});

    // TODO: by default should just be next to the executable
    std::filesystem::path settings_path = R"(C:\Users\thejo\OneDrive\Documents\PortalEngine\test\settings.json)";

    // TODO: fetch this information from environment variables or CLI?
    Settings::init(SettingsArchiveType::Json, settings_path);
}

void initialize_logger()
{
    // TODO: user env variables as well?
    auto log_level_string = Settings::get().get_setting<std::string>(LOG_LEVEL_ENTRY);
    if (log_level_string)
    {
        const auto log_level = Log::level_from_string(*log_level_string);
        Log::set_default_log_level(log_level);
    }
    Settings::get().debug_print();
}

ApplicationSpecification make_application_spec()
{
    auto& settings = Settings::get();

    const auto name= settings.get_setting<std::string>("application.name");
    const auto width = settings.get_setting<size_t>("application.window.width");
    const auto height = settings.get_setting<size_t>("application.window.height");
    const auto resources_path = settings.get_setting<std::filesystem::path>("application.resources-path");
    const auto scheduler_worker_num = settings.get_setting<int32_t>("application.scheduler-threads");

    return ApplicationSpecification{
        .name = STRING_ID(name.value()),
        .width = width.value(),
        .height = height.value(),
        .resources_path = resources_path.value(),
        .scheduler_worker_num = scheduler_worker_num.value(),
    };
}

int main()
{
    try
    {
        initialize_settings();
        initialize_logger();

        const auto spec = make_application_spec();

        Application app{spec};
        app.run();
    }
    catch (std::exception& e)
    {
        LOG_FATAL("Unhandled exception: {}", e.what());
    }
    catch (...)
    {
        LOG_FATAL("Unhandled unknown exception");
    }

    Settings::shutdown();
    Log::shutdown();
}
