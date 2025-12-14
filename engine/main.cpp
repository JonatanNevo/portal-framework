//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/application/application.h"
#include "portal/application/entry_point.h"

#include "portal/core/files/file_system.h"
#include "portal/engine/settings.h"
#include "glaze/core/reflect.hpp"
#include "portal/core/string_utils.h"
#include "portal/engine/engine.h"
#include "portal/engine/ecs/registry.h"
#include "portal/engine/resources/resources/composite.h"
#include "portal/engine/systems/scene_rendering_system.h"
#include "portal/engine/systems/transform_hierarchy_system.h"

using namespace portal;

constexpr auto LOG_LEVEL_ENTRY = "log-level";

void initialize_settings()
{
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
        const auto log_level = portal::from_string<Log::LogLevel>(*log_level_string);
        Log::set_default_log_level(log_level);
    }
    Settings::get().debug_print();
}

ApplicationProperties make_application_properties()
{
    auto& settings = Settings::get();

    const auto name = settings.get_setting<std::string>("application.name");
    const auto width = settings.get_setting<size_t>("application.window.width");
    const auto height = settings.get_setting<size_t>("application.window.height");
    const auto resources_path = settings.get_setting<std::filesystem::path>("application.resources-path");
    const auto scheduler_worker_num = settings.get_setting<int32_t>("application.scheduler-threads");

    return ApplicationProperties{
        .name = STRING_ID(name.value()),
        .width = width.value(),
        .height = height.value(),
        .resources_path = resources_path.value(),
        .scheduler_worker_num = scheduler_worker_num.value(),
    };
}

std::unique_ptr<Application> portal::create_application(int, char**)
{
    initialize_settings();
    initialize_logger();

    const auto prop = make_application_properties();
    auto engine =  std::make_unique<Engine>(prop);
    // TODO: Should not be here

    auto& engine_context = engine->get_engine_context();
    [[maybe_unused]] auto composite = engine_context.get_resource_registry().immediate_load<Composite>(STRING_ID("game/ABeautifulGame"));
    auto scene = engine_context.get_resource_registry().get<Scene>(STRING_ID("game/gltf-Scene-Scene"));
    PORTAL_ASSERT(scene.get_state() == ResourceState::Loaded, "Failed to load scene");

    // engine->setup_scene(scene);

    return engine;
}
