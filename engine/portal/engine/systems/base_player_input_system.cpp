//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "base_player_input_system.h"

#include "portal/input/input_manager.h"

namespace portal
{
void BasePlayerInputSystem::execute(ecs::Registry& registry)
{
    const auto player_group = group(registry);
    PORTAL_ASSERT(player_group.size() == 1, "Expected exactly one player entity");

    for (auto&& [entity_id, input_component, controller, _] : player_group.each())
    {
        auto* input = input_component.input_manager;
        PORTAL_ASSERT(input != nullptr, "Invalid input component state");

        if (input && input->is_key_pressed(Key::RightMouseButton))
        {
            disable_mouse(input);
            controller.mark_as_moving();

            // TODO: use actions instead of raw keys
            // TODO: check if its better to use callbacks here?
            if (input->is_key_pressed(Key::E))
                controller.move_up(1.f);
            if (input->is_key_pressed(Key::Q))
                controller.move_up(-1.f);
            if (input->is_key_pressed(Key::W))
                controller.move_forward(1.f);
            if (input->is_key_pressed(Key::S))
                controller.move_forward(-1.f);
            if (input->is_key_pressed(Key::D))
                controller.move_right(1.f);
            if (input->is_key_pressed(Key::A))
                controller.move_right(-1.f);

            controller.look_to(input->get_mouse_position());
        }
        else
        {
            enable_mouse(input);
            controller.mark_as_stopped_moving();
        }
    }
}

void BasePlayerInputSystem::enable_mouse(const InputManager* input)
{
    input->set_cursor_mode(CursorMode::Normal);
}

void BasePlayerInputSystem::disable_mouse(const InputManager* input)
{
    input->set_cursor_mode(CursorMode::Locked);
}
} // portal
