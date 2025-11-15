//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "input.h"

#include "portal/core/log.h"

namespace portal::ng
{

static auto logger = Log::get_logger("Input");


Input::Input()
{
    // We start from 1 to ignore `Key::Invalid`
    for (int i = 1; i < std::to_underlying(Key::Max); i++)
    {
        key_states[static_cast<Key>(i)] = KeyData{.key = static_cast<Key>(i)};
    }
}

void Input::update_key_state(const Key key, const KeyState state)
{
    auto& key_data = key_states[key];
    key_data.previous_state = key_data.state;
    key_data.state = state;
}

void Input::update_modifiers(const KeyModifierFlag new_modifiers)
{
    modifiers = new_modifiers;
}

void Input::process_events(float delta_time)
{
}

void Input::map_action(const Key key, const StringId& action, const KeyModifierFlag modifiers)
{
    key_action_mappings.emplace_back(
        ActionKeyMapping{
            .action = action,
            .modifiers = modifiers,
            .key = key
        }
        );
}

void Input::bind_action(const StringId& action, const KeyState event, ActionBinding::action_function&& func)
{
    action_bindings.emplace_back(ActionBinding{action, event, std::move(func)});
}


bool Input::is_key_pressed(const Key key) const
{
    return key_states.at(key).state == KeyState::Pressed || key_states.at(key).state == KeyState::Repeat;
}

bool Input::is_key_released(const Key key) const
{
    return key_states.at(key).state == KeyState::Released;
}

bool Input::is_key_repeating(const Key key) const
{
    return key_states.at(key).state == KeyState::Repeat;
}

void Input::transition_key_states()
{
    for (const auto& [key, data] : key_states)
    {
        if (data.state == KeyState::Pressed)
            update_key_state(key, KeyState::Repeat);
    }
}
}
