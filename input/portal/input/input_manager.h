//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <llvm/ADT/DenseMap.h>
#include <portal/core/events/event.h>
#include <portal/application/modules/module.h>

#include "key_data.h"
#include "portal/input/input_event_consumer.h"

namespace portal
{
class InputManager final : public Module<>, public InputEventConsumer
{
public:
    explicit InputManager(ModuleStack& stack, const std::optional<std::function<void(Event&)>>& event_callback = std::nullopt);

    [[nodiscard]] bool is_key_pressed(Key key) const;
    [[nodiscard]] bool is_key_released(Key key) const;
    [[nodiscard]] bool is_key_repeating(Key key) const;

    [[nodiscard]] glm::vec2 get_mouse_position() const { return mouse_position; }
    [[nodiscard]] glm::vec2 get_mouse_scroll() const { return mouse_scroll; }

    void report_key_action(Key key, KeyState state, std::optional<KeyModifierFlag> modifiers) override;
    void report_axis_change(Axis axis, glm::vec2 value) override;

    void set_cursor_mode(CursorMode mode) const;

private:
    /**
     * Transitions and prepares the key states for the next frame.
     * Moves pressed to repeat
     */
    void transition_key_states();

private:
    std::function<void(Event&)> event_callback;

    llvm::DenseMap<Key, KeyData> key_states;
    KeyModifierFlag active_modifiers = KeyModifierBits::None;
    glm::vec2 mouse_position = glm::vec2(0.0f);
    glm::vec2 mouse_scroll = glm::vec2(0.0f);
};
} // portal
