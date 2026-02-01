//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "base_player_input_system.h"

#include "portal/input/input_manager.h"

namespace portal
{

BasePlayerInputSystem::BasePlayerInputSystem(InputManager& input_manager): input_manager(input_manager)
{}

void BasePlayerInputSystem::connect(ecs::Registry& registry, entt::dispatcher& dispatcher)
{
    dispatcher.sink<KeyPressedEvent>().connect<&BasePlayerInputSystem::on_key_pressed>(this);
    dispatcher.sink<KeyReleasedEvent>().connect<&BasePlayerInputSystem::on_key_released>(this);
    dispatcher.sink<MouseMovedEvent>().connect<&BasePlayerInputSystem::on_mouse_moved>(this);

    for (auto&& [entity_id, controller, _] : group(registry).each())
    {
        controller.mark_as_moving();
    }

    // TODO: is this the best way of doing this?
    mouse_position = input_manager.get_mouse_position();
}

void BasePlayerInputSystem::disconnect(ecs::Registry& registry, entt::dispatcher& dispatcher)
{
    dispatcher.sink<KeyPressedEvent>().disconnect<&BasePlayerInputSystem::on_key_pressed>(this);
    dispatcher.sink<KeyReleasedEvent>().disconnect<&BasePlayerInputSystem::on_key_released>(this);
    dispatcher.sink<MouseMovedEvent>().disconnect<&BasePlayerInputSystem::on_mouse_moved>(this);

    for (auto&& [entity_id, controller, _] : group(registry).each())
    {
        controller.mark_as_stopped_moving();
    }
}

void BasePlayerInputSystem::execute(ecs::Registry& registry) const
{
    const auto player_group = group(registry);
    PORTAL_ASSERT(player_group.size() == 1, "Expected exactly one player entity");

    for (auto&& [entity_id, controller, _] : player_group.each())
    {
        if (!controller.should_move_enabled())
            return;

        if (move_up) controller.move_up(1.f);
        if (move_down) controller.move_up(-1.f);
        if (move_forward) controller.move_forward(1.f);
        if (move_backward) controller.move_forward(-1.f);
        if (move_right) controller.move_right(1.f);
        if (move_left) controller.move_right(-1.f);

        controller.look_to(mouse_position);
    }
}

void BasePlayerInputSystem::on_key_pressed(const KeyPressedEvent& event)
{
    switch (event.key)
    {
    case Key::W: move_forward = true;
        break;
    case Key::S: move_backward = true;
        break;
    case Key::A: move_left = true;
        break;
    case Key::D: move_right = true;
        break;
    case Key::E: move_up = true;
        break;
    case Key::Q: move_down = true;
        break;
    default: break;
    }
}

void BasePlayerInputSystem::on_key_released(const KeyReleasedEvent& event)
{
    switch (event.key)
    {
    case Key::W: move_forward = false;
        break;
    case Key::S: move_backward = false;
        break;
    case Key::A: move_left = false;
        break;
    case Key::D: move_right = false;
        break;
    case Key::E: move_up = false;
        break;
    case Key::Q: move_down = false;
        break;
    default: break;
    }
}

void BasePlayerInputSystem::on_mouse_moved(const MouseMovedEvent& event)
{
    mouse_position = event.position;
}
} // portal
