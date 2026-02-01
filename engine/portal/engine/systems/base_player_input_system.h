//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/components/base.h"
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/ecs/system.h"
#include "portal/input/input_events.h"

namespace portal
{
class BasePlayerInputSystem : public ecs::System<
        BasePlayerInputSystem,
        ecs::Views<BaseCameraController>,
        ecs::Views<PlayerTag>
    >
{
public:
    BasePlayerInputSystem(InputManager& input_manager);

    void connect(ecs::Registry& registry, entt::dispatcher& dispatcher) override;
    void disconnect(ecs::Registry& registry, entt::dispatcher& dispatcher) override;

    void execute(ecs::Registry& registry) const;

    void on_key_pressed(const KeyPressedEvent& event);
    void on_key_released(const KeyReleasedEvent& event);
    void on_mouse_moved(const MouseMovedEvent& event);

    [[nodiscard]] static StringId get_name() { return STRING_ID("Base Player Input"); };

private:
    InputManager& input_manager;

    bool move_forward = false;
    bool move_backward = false;
    bool move_left = false;
    bool move_right = false;
    bool move_up = false;
    bool move_down = false;

    glm::vec2 mouse_position{};
};
} // portal
