//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/application/application.h"
#include "portal/application/entry_point.h"

#include "portal/engine/engine.h"
#include "portal/engine/components/base.h"
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/ecs/registry.h"
#include "portal/engine/resources/resources/composite.h"

using namespace portal;

std::unique_ptr<Application> portal::create_application(int, char**)
{
    const auto prop = ApplicationProperties::from_settings();
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

    return engine;
}
