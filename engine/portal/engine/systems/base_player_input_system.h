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
    static void execute(ecs::Registry& registry);

    static void enable_mouse(const InputManager* input);
    static void disable_mouse(const InputManager* input);

    [[nodiscard]] static StringId get_name() { return STRING_ID("Base Player Input"); };
};
} // portal
