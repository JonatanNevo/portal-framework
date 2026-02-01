//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "input_manager.h"

#include "portal/core/log.h"
#include "input_events.h"

namespace portal
{
static auto logger = Log::get_logger("Input");

InputManager::InputManager(ModuleStack& stack, entt::dispatcher& engine_dispatcher, entt::dispatcher& input_dispatcher)
    : Module<>(stack, STRING_ID("Input")),
      engine_dispatcher(engine_dispatcher),
      input_dispatcher(input_dispatcher)
{
    // We start from 1 to ignore `Key::Invalid`
    for (int i = 1; i < std::to_underlying(Key::Max); i++)
    {
        key_states[static_cast<Key>(i)] = KeyData{.key = static_cast<Key>(i)};
    }

    engine_dispatcher.sink<ReportKeyActionEvent>().connect<&InputManager::report_key_action>(this);
    engine_dispatcher.sink<ReportAnalogAxisEvent>().connect<&InputManager::report_axis_change>(this);
}

bool InputManager::is_key_pressed(const Key key) const
{
    return key_states.at(key).state == KeyState::Pressed || key_states.at(key).state == KeyState::Repeat;
}

bool InputManager::is_key_released(const Key key) const
{
    return key_states.at(key).state == KeyState::Released;
}

bool InputManager::is_key_repeating(const Key key) const
{
    return key_states.at(key).state == KeyState::Repeat;
}

void InputManager::report_key_action(ReportKeyActionEvent event)
{
    active_modifiers = event.modifiers.value_or(active_modifiers);

    auto& key_data = key_states[event.key];
    key_data.previous_state = key_data.state;
    key_data.state = event.state;

    // TODO: accumulate actions between process calls and make all events in one location
    switch (event.state)
    {
    case KeyState::Pressed:
        input_dispatcher.enqueue<KeyPressedEvent>(event.key, active_modifiers);
        break;
    case KeyState::Released:
        input_dispatcher.enqueue<KeyReleasedEvent>(event.key);
        break;
    case KeyState::Repeat:
        input_dispatcher.enqueue<KeyRepeatEvent>(event.key, active_modifiers);
        break;
    }
}

void InputManager::report_axis_change(ReportAnalogAxisEvent event)
{
    // TODO: accumulate changes between process calls and make all events in one location
    switch (event.axis)
    {
    case Axis::Mouse:
        mouse_position = event.value;
        input_dispatcher.enqueue<MouseMovedEvent>(event.value);
        break;
    case Axis::MouseScroll:
        mouse_scroll = event.value;
        input_dispatcher.enqueue<MouseScrolledEvent>(event.value);
        break;
    }
}

void InputManager::set_cursor_mode(CursorMode mode) const
{
    engine_dispatcher.enqueue<SetMouseCursorEvent>(mode);
}


void InputManager::transition_key_states()
{
    for (const auto& [key, data] : key_states)
    {
        if (data.state == KeyState::Pressed)
            report_key_action(ReportKeyActionEvent{key, KeyState::Repeat, std::nullopt});
    }
}
}
