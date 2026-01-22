//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/components/base.h"
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/ecs/system.h"

namespace portal
{
class BasePlayerInputSystem : public ecs::System<
        BasePlayerInputSystem,
        ecs::Owns<InputComponent>,
        ecs::Views<BaseCameraController>,
        ecs::Views<PlayerTag>
    >
{
public:
    BasePlayerInputSystem(InputManager& input_manager);

    static void execute(ecs::Registry& registry);

    static void enable_mouse(const InputManager* input);
    static void disable_mouse(const InputManager* input);

    void on_component_added(Entity entity, InputComponent& input_component) const;
    void on_component_changed(Entity entity, InputComponent& input_component) const;

    [[nodiscard]] static StringId get_name() { return STRING_ID("Base Player Input"); };

private:
    InputManager& input_manager;
};
} // portal
