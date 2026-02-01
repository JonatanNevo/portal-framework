//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <entt/signal/dispatcher.hpp>
#include <llvm/ADT/DenseMap.h>
#include <portal/application/modules/module.h>


#include "input_events.h"
#include "key_data.h"

namespace portal
{
/**
 * Cross-platform input abstraction managing keyboard and mouse state.
 *
 * Provides unified access to keyboard and mouse input across all platforms. Supports both
 * event-driven handling (via input_dispatcher events) and polling (via is_key_pressed, etc.).
 *
 * ## Key State Lifecycle
 * - **Released** - Key is up (default state)
 * - **Pressed** - Key just went down this frame (fires KeyPressedEvent)
 * - **Repeat** - Key held down from previous frame
 *
 * is_key_pressed() returns true for both Pressed and Repeat states. To detect initial press only:
 * @code
 * if (is_key_pressed(Key::Space) && !is_key_repeating(Key::Space)) {
 *     player.jump();  // Only on initial press
 * }
 * @endcode
 *
 * ## Usage Examples
 * @code
 * // Polling in a system
 * if (input->is_key_pressed(Key::W))
 *     transform.position.y += speed * dt;
 *
 * // Event handling
 * dispatcher.sink<KeyPressedEvent>().connect<&on_key_pressed>();
 *
 * // Cursor control
 * input->set_cursor_mode(CursorMode::Locked);
 * @endcode
 *
 * @see Key, KeyState, KeyPressedEvent, KeyReleasedEvent, MouseMovedEvent, MouseScrolledEvent
 */
class InputManager final : public Module<>
{
public:
    /**
     * Constructs the InputManager and initializes input state tracking.
     *
     * @param stack Module system integration
     * @param engine_dispatcher Engine-level events (e.g., SetMouseCursorEvent to Window)
     * @param input_dispatcher Input events (KeyPressedEvent, MouseMovedEvent, etc.)
     */
    InputManager(ModuleStack& stack, entt::dispatcher& engine_dispatcher, entt::dispatcher& input_dispatcher);

    /**
     * Checks if a key is currently down (Pressed or Repeat state).
     *
     * Returns true for both initial press and held keys. For initial press only, use:
     * `is_key_pressed(key) && !is_key_repeating(key)`
     *
     * @param key Keyboard key or mouse button to check
     * @return true if key is currently down
     */
    [[nodiscard]] bool is_key_pressed(Key key) const;

    /**
     * Checks if a key is currently up (Released state).
     *
     * @param key Keyboard key or mouse button to check
     * @return true if key is not currently down
     */
    [[nodiscard]] bool is_key_released(Key key) const;

    /**
     * Checks if a key is in Repeat state (held from previous frame).
     *
     * Useful for distinguishing initial press from continued hold.
     *
     * @param key Keyboard key or mouse button to check
     * @return true if key is being held down
     */
    [[nodiscard]] bool is_key_repeating(Key key) const;

    /**
     * Gets the current mouse position in window coordinates.
     *
     * @return Mouse position (x, y) with (0,0) at top-left
     */
    [[nodiscard]] glm::vec2 get_mouse_position() const { return mouse_position; }

    /**
     * Gets the most recent mouse scroll offset (not accumulated).
     *
     * @return Scroll offset (x = horizontal, y = vertical)
     */
    [[nodiscard]] glm::vec2 get_mouse_scroll() const { return mouse_scroll; }

    /**
     * Reports a key state change from platform layer (Window implementations only).
     *
     * Updates internal state and dispatches KeyPressedEvent/KeyReleasedEvent/KeyRepeatEvent.
     *
     * @param event Contains key, state, and active modifiers
     */
    void report_key_action(ReportKeyActionEvent event);

    /**
     * Reports analog axis change from platform layer (Window implementations only).
     *
     * Updates cached values and dispatches MouseMovedEvent/MouseScrolledEvent.
     *
     * @param event Contains axis (Mouse or MouseScroll) and value
     */
    void report_axis_change(ReportAnalogAxisEvent event);

    /**
     * Requests cursor mode change via SetMouseCursorEvent.
     *
     * Modes: Normal (visible), Hidden (invisible but free), Locked (invisible and confined).
     * When locked, mouse position is fixed at the window center.
     *
     * @param mode Desired cursor mode
     */
    void set_cursor_mode(CursorMode mode) const;

private:
    /**
     * Transitions and prepares the key states for the next frame.
     * Moves pressed to repeat
     */
    void transition_key_states();

private:
    entt::dispatcher& engine_dispatcher;
    entt::dispatcher& input_dispatcher;

    llvm::DenseMap<Key, KeyData> key_states;
    KeyModifierFlag active_modifiers = KeyModifierBits::None;
    glm::vec2 mouse_position = glm::vec2(0.0f);
    glm::vec2 mouse_scroll = glm::vec2(0.0f);
};
} // namespace portal
