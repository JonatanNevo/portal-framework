//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/ecs/system.h"

namespace portal
{
class BaseCameraSystem : public ecs::System<
        BaseCameraSystem,
        ecs::Owns<BaseCameraController>,
        ecs::Views<CameraComponent>,
        ecs::Views<TransformComponent>
    >
{
public:
    void connect(ecs::Registry& registry, entt::dispatcher& dispatcher) override;
    void disconnect(ecs::Registry& registry, entt::dispatcher& dispatcher) override;

    static void execute(FrameContext& frame, ecs::Registry& registry);

    static void on_component_added(Entity entity, CameraComponent& camera_component);
    static void on_component_changed(Entity entity, CameraComponent& camera_component);

    [[nodiscard]] static StringId get_name() { return STRING_ID("Base Camera System"); };
};
} // portal
