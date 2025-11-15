//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "llvm/ADT/DenseMap.h"
#include "portal/core/strings/string_id.h"
#include "portal/input/new/action.h"
#include "portal/input/new/key_data.h"

namespace portal::ng
{

class Input
{
public:
    Input();

    /**
     * Report a new key event
     */
    void update_key_state(Key key, KeyState state);
    void update_modifiers(KeyModifierFlag modifiers);

    /**
     * Process all pending key events
     * @param delta_time Time passed since last process class (last frame)
     */
    void process_events(float delta_time);

    void map_action(Key key, const StringId& action, KeyModifierFlag modifiers = KeyModifierBits::None);
    void bind_action(const StringId& action, KeyState event, ActionBinding::action_function&& func);

    [[nodiscard]] bool is_key_pressed(Key key) const;
    [[nodiscard]] bool is_key_released(Key key) const;
    [[nodiscard]] bool is_key_repeating(Key key) const;

private:
    /**
     * Transitions and prepares the key states for the next frame.
     * Moves pressed to repeat
     */
    void transition_key_states();

private:
    llvm::SmallVector<ActionKeyMapping> key_action_mappings;
    llvm::SmallVector<ActionBinding> action_bindings;

    llvm::DenseMap<Key, KeyData> key_states;
    KeyModifierFlag modifiers = KeyModifierBits::None;
};

} // portal