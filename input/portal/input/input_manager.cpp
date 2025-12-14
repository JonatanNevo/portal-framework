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

InputManager::InputManager(ModuleStack& stack, const std::optional<std::function<void(Event&)>>& event_callback) : Module<>(stack, STRING_ID("Input"))
{
    if (event_callback)
    {
        this->event_callback = event_callback.value();
    }
    else
    {
        this->event_callback = [](const Event& e)
        {
            LOGGER_ERROR("Unprocessed event: {}", e.to_string());
        };
    }

    // We start from 1 to ignore `Key::Invalid`
    for (int i = 1; i < std::to_underlying(Key::Max); i++)
    {
        key_states[static_cast<Key>(i)] = KeyData{.key = static_cast<Key>(i)};
    }
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

void InputManager::report_key_action(const Key key, const KeyState state, const std::optional<KeyModifierFlag> modifiers)
{
    active_modifiers = modifiers.value_or(active_modifiers);

    auto& key_data = key_states[key];
    key_data.previous_state = key_data.state;
    key_data.state = state;

    // TODO: accumulate actions between process calls and make all events in one location
    switch (state)
    {
    case KeyState::Pressed:
        {
            KeyPressedEvent event(key, active_modifiers);
            event_callback(event);
            break;
        }
    case KeyState::Released:
        {
            KeyReleasedEvent event(key);
            event_callback(event);
            break;
        }
    case KeyState::Repeat:
        {
            KeyRepeatEvent event(key, active_modifiers);
            event_callback(event);
            break;
        }
    }
}

void InputManager::report_axis_change(Axis axis, glm::vec2 value)
{
    // TODO: accumulate changes between process calls and make all events in one location
    switch (axis)
    {
    case Axis::Mouse:
        {
            mouse_position = value;
            MouseMovedEvent event(value);
            event_callback(event);
            break;
        }
    case Axis::MouseScroll:
        {
            mouse_scroll = value;
            MouseScrolledEvent event(value);
            event_callback(event);
            break;
        }
    }
}

void InputManager::set_cursor_mode(const CursorMode mode) const
{
    SetMouseCursorEvent event(mode);
    event_callback(event);
}

void InputManager::transition_key_states()
{
    for (const auto& [key, data] : key_states)
    {
        if (data.state == KeyState::Pressed)
            report_key_action(key, KeyState::Repeat, std::nullopt);
    }
}
}
