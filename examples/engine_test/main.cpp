//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/application/application.h"
#include "portal/application/entry_point.h"

#include "portal/core/files/file_system.h"
#include "portal/engine/settings.h"
#include "glaze/core/reflect.hpp"
#include "portal/core/strings/string_utils.h"
#include "portal/engine/engine.h"
#include "portal/engine/components/base.h"
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/ecs/registry.h"
#include "portal/engine/resources/resources/composite.h"

using namespace portal;

constexpr auto LOG_LEVEL_ENTRY = "log-level";

void initialize_settings()
{
    Settings::init(SettingsArchiveType::Json, "settings.json");
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

    const auto name = settings.get_setting<std::string>("name");
    const auto width = settings.get_setting<size_t>("application.window.width");
    const auto height = settings.get_setting<size_t>("application.window.height");

    return ApplicationProperties{
        .name = STRING_ID(name.value()),
        .width = width.value(),
        .height = height.value()
    };
}

std::unique_ptr<Application> portal::create_application(int, char**)
{
    initialize_settings();
    initialize_logger();

    const auto prop = make_application_properties();
    auto engine = std::make_unique<Engine>(prop);
    // TODO: Should not be here

    auto& engine_context = engine->get_engine_context();
    [[maybe_unused]] auto composite = engine_context.get_resource_registry().immediate_load<Composite>(STRING_ID("game/ABeautifulGame"));
    auto scene = engine_context.get_resource_registry().get<Scene>(STRING_ID("game/gltf-Scene-Scene"));
    PORTAL_ASSERT(scene.get_state() == ResourceState::Loaded, "Failed to load scene");

    // Serialize camera as well
    auto camera = scene->get_registry().create_entity(STRING_ID("Camera"));
    camera.add_component<PlayerTag>();
    camera.add_component<InputComponent>();
    auto& controller = camera.add_component<BaseCameraController>();
    auto& camera_comp = camera.add_component<CameraComponent>();
    camera.add_component<MainCameraTag>();

    auto& transform = camera.get_component<TransformComponent>();
    transform.set_translation(glm::vec3(-0.51f, 0.4f, 0.74f));

    camera_comp.calculate_view(transform.get_translation(), controller.forward_direction);

    engine->setup_scene(scene);


    return engine;
}
