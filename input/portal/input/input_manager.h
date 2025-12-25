//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <llvm/ADT/DenseMap.h>
#include <portal/application/modules/module.h>
#include <portal/core/events/event.h>


#include "key_data.h"
#include "portal/input/input_event_consumer.h"

namespace portal
{
/**
 * Cross-platform input abstraction managing keyboard and mouse state.
 *
 * InputManager is the central hub of Portal's input system, providing unified access to
 * keyboard and mouse input across all supported platforms (Windows, macOS, Linux). It abstracts
 * platform-specific input APIs into a consistent interface using Portal's
 * unified Key enum, allowing game code to query input state and respond to input events without
 * any platform-specific code.
 *
 * InputManager supports two complementary approaches to input handling:
 *
 * **Event-Driven Pattern** - Register event handlers to respond to input changes as they occur.
 * Best for UI interactions, and one-shot actions (pause menu, screenshot capture):
 *
 * @code
 * void on_event(Event& event) override {
 *     EventRunner runner(event);
 *
 *     runner.run_on<KeyPressedEvent>([](const KeyPressedEvent& e) {
 *         if (e.get_key() == Key::Escape) {
 *             game.show_pause_menu();
 *             return true;  // Mark event as handled
 *         }
 *         return false;
 *     });
 *
 *     runner.run_on<MouseScrolledEvent>([](const MouseScrolledEvent& e) {
 *         camera.zoom(e.get_y_offset());
 *         return true;
 *     });
 * }
 * @endcode
 *
 * **Polling Pattern** - Query input state each frame during system updates. Best for continuous
 * actions (player movement, camera control) and gameplay logic that runs in sync with the
 * game loop:
 *
 * @code
 * // In an ECS system update
 * void update(ecs::Registry& registry) {
 *     for (auto&& [entity, input_comp, transform] : registry.view<InputComponent, Transform>().each()) {
 *         auto* input = input_comp.input_manager;
 *
 *         // Continuous movement while key held
 *         if (input->is_key_pressed(Key::W))
 *             transform.position.y += speed * delta_time;
 *
 *         // Check for initial press only (not repeat)
 *         if (input->is_key_pressed(Key::SpaceBar) && !input->is_key_repeating(Key::SpaceBar))
 *             player.jump();
 *
 *         // Mouse-based camera control
 *         auto [x, y] = input->get_mouse_position();
 *         camera.update_from_mouse(x, y);
 *     }
 * }
 * @endcode
 *
 * ## Key State Lifecycle
 *
 * Each key progresses through a well-defined state lifecycle:
 *
 * - **Released** - Key is up (default state)
 * - **Pressed** - Key just went down THIS frame (fires KeyPressedEvent)
 * - **Repeat** - Key is being held down (transitioned from Pressed after one frame)
 *
 * The transition from Pressed to Repeat happens automatically via transition_key_states(),
 * called once per frame. This ensures that is_key_pressed() returns true for BOTH Pressed
 * and Repeat states (meaning "currently down"), while allowing detection of initial press
 * by checking `is_key_pressed(key) && !is_key_repeating(key)`.
 *
 *
 * ## ECS Integration
 *
 * Entities access input through the InputComponent, which holds a pointer to the InputManager.
 * The SystemOrchestrator automatically patches InputComponent instances during scene loading:
 *
 * @code
 * // Entity setup
 * auto player = registry.create_entity(STRING_ID("Player"));
 * player.add_component<InputComponent>();  // Pointer injected by SystemOrchestrator
 *
 * // In a system
 * void execute(ecs::Registry& registry) {
 *     for (auto entity : registry.view<InputComponent>()) {
 *         auto* input = entity.get_component<InputComponent>().input_manager;
 *         // Use input->is_key_pressed(), etc.
 *     }
 * }
 * @endcode
 *
 * ## Cursor Control
 *
 * The set_cursor_mode() method controls cursor visibility and lock state. Instead of calling
 * platform APIs directly, it dispatches a SetMouseCursorEvent through the event system. The
 * Window handles this event and applies the mode change via platform calls:
 *
 * @code
 * // Lock cursor for FPS camera control
 * input->set_cursor_mode(CursorMode::Locked);  // Invisible and confined to window
 *
 * // Show cursor for menu navigation
 * input->set_cursor_mode(CursorMode::Normal);  // Visible and free-moving
 * @endcode
 *
 * ## Modifier Keys
 *
 * Key events (KeyPressedEvent, KeyRepeatEvent) include active modifier flags (Shift, Ctrl,
 * Alt, System/Command, CapsLock, NumLock) as a type-safe bitflag. This enables keyboard
 * shortcut detection:
 *
 * @code
 * runner.run_on<KeyPressedEvent>([](const KeyPressedEvent& e) {
 *     if (e.get_key() == Key::S && e.get_modifiers().has(KeyModifierBits::Ctrl)) {
 *         game.save();  // Ctrl+S detected
 *         return true;
 *     }
 *     return false;
 * });
 * @endcode
 *
 * @see InputEventConsumer - Platform interface for reporting hardware events
 * @see Key - Unified keyboard and mouse button enumeration
 * @see KeyState - Per-key state values (Pressed, Released, Repeat)
 * @see InputComponent - ECS component for entity-based input access
 * @see KeyPressedEvent, KeyReleasedEvent, KeyRepeatEvent - Keyboard event types
 * @see MouseMovedEvent, MouseScrolledEvent - Mouse event types
 */
class InputManager final : public Module<>, public InputEventConsumer
{
public:
    /**
     * Constructs the InputManager and initializes input state tracking.
     *
     *
     * @param stack The ModuleStack for module system integration
     * @param event_callback Optional callback function for dispatching input events. When input
     *                       state changes (key press, mouse move, etc.), the InputManager generates
     *                       typed events (KeyPressedEvent, MouseMovedEvent, etc.) and invokes this
     *                       callback to propagate them through the engine's event system. Typically
     *                       set to Engine::on_event() during engine initialization. If not provided,
     *                       defaults to logging unprocessed events.
     */
    InputManager(ModuleStack& stack, const std::optional<std::function<void(Event&)>>& event_callback = std::nullopt);

    /**
     * Checks if a specific key is currently down (pressed or held).
     *
     * Returns true if the key state is EITHER Pressed OR Repeat, meaning the key is currently
     * down regardless of whether it just went down this frame or is being held. This is the
     * correct method for continuous actions like player movement.
     *
     * To detect if a key was JUST pressed this frame (not held from previous frames), combine
     * with is_key_repeating():
     * @code
     * if (is_key_pressed(Key::SpaceBar) && !is_key_repeating(Key::SpaceBar)) {
     *     // Key was just pressed this frame - trigger jump
     * }
     * @endcode
     *
     * @param key The key to check (can be keyboard key or mouse button)
     * @return true if the key is currently down (Pressed or Repeat state), false otherwise
     * @see is_key_repeating() - Check if key is in Repeat state specifically
     * @see is_key_released() - Check if key is up
     */
    [[nodiscard]] bool is_key_pressed(Key key) const;

    /**
     * Checks if a specific key is currently up (not pressed).
     *
     * Returns true only if the key state is Released, meaning the key is not currently down.
     * Most game code checks is_key_pressed() rather than is_key_released(), as the latter
     * is true for the majority of keys most of the time.
     *
     * @param key The key to check (can be keyboard key or mouse button)
     * @return true if the key is in Released state, false otherwise
     * @see is_key_pressed() - Check if key is currently down
     */
    [[nodiscard]] bool is_key_released(Key key) const;

    /**
     * Checks if a specific key is in the Repeat state (held down past initial frame).
     *
     * Returns true only if the key state is Repeat, meaning the key was pressed in a previous
     * frame and is still being held. This allows distinguishing between "just pressed this frame"
     * and "held down from previous frame".
     *
     * Common pattern for detecting initial press only:
     * @code
     * if (is_key_pressed(key) && !is_key_repeating(key)) {
     *     // Key was just pressed THIS frame
     * }
     * @endcode
     *
     * Note: is_key_pressed() returns true for BOTH Pressed and Repeat states, so checking
     * is_key_repeating() is the way to differentiate initial press from continued hold.
     *
     * @param key The key to check (can be keyboard key or mouse button)
     * @return true if the key is in Repeat state (held down), false otherwise
     * @see is_key_pressed() - Returns true for both Pressed and Repeat states
     */
    [[nodiscard]] bool is_key_repeating(Key key) const;

    /**
     * Gets the current mouse cursor position in window coordinates.
     *
     * Returns the absolute position of the mouse cursor within the window, with (0,0) typically
     * at the top-left corner.
     *
     * When the cursor is locked (CursorMode::Locked), the position still updates based on raw
     * mouse motion, though the cursor itself is invisible and confined to the window.
     *
     * @return The current mouse position as a vec2 (x, y in window space)
     * @see set_cursor_mode() - Control cursor visibility and lock state
     */
    [[nodiscard]] glm::vec2 get_mouse_position() const { return mouse_position; }

    /**
     * Gets the most recent mouse scroll wheel offset.
     *
     * Returns the scroll offset from the most recent scroll event. The y component represents
     * vertical scrolling (positive = scroll up, negative = scroll down), while the x component
     * represents horizontal scrolling on devices that support it (trackpads, horizontal scroll
     * wheels).
     *
     * @note This returns the offset from the LAST scroll event, not an accumulated value. If you
     * need to accumulate scroll over multiple frames, you must track it yourself. The value is
     * updated each time a MouseScrolledEvent is received.
     *
     * @return The most recent scroll offset as a vec2 (x = horizontal, y = vertical)
     */
    [[nodiscard]] glm::vec2 get_mouse_scroll() const { return mouse_scroll; }

    /**
     * Reports a key state change from the platform layer (InputEventConsumer interface).
     *
     * This method is called by platform implementations (e.g., GlfwWindow GLFW callbacks) when
     * a keyboard key or mouse button changes state. It should NOT be called by game code directly.
     *
     * The method updates internal key state tracking, stores modifier flags, and generates the
     * appropriate input event (KeyPressedEvent, KeyReleasedEvent, or KeyRepeatEvent) which is
     * dispatched through the event_callback.
     *
     * @param key The key that changed state (already translated from platform codes to Portal's Key enum)
     * @param state The new state (Pressed, Released, or Repeat)
     * @param modifiers Optional bitflags for active modifiers (Shift, Ctrl, Alt, etc.) at the time
     *                  of the key action. Included in generated KeyPressedEvent and KeyRepeatEvent.
     * @note This is part of the InputEventConsumer interface for platform abstraction, you should not call it directly
     * @see InputEventConsumer - Platform integration interface
     */
    void report_key_action(Key key, KeyState state, std::optional<KeyModifierFlag> modifiers) override;

    /**
     * Reports an analog axis change from the platform layer (InputEventConsumer interface).
     *
     * This method is called by platform implementations when an analog input changes, such as
     * mouse cursor movement or scroll wheel rotation. It should NOT be called by game code directly.
     *
     * The method updates the cached axis value (mouse_position or mouse_scroll) and generates
     * the appropriate event (MouseMovedEvent or MouseScrolledEvent) which is dispatched through
     * the event_callback.
     *
     * @param axis The axis that changed (currently Axis::Mouse for cursor movement or
     *             Axis::MouseScroll for scroll wheel)
     * @param value The new axis value. For Mouse, this is absolute window-space position (x, y).
     *              For MouseScroll, this is the scroll offset (x = horizontal, y = vertical).
     * @note This is part of the InputEventConsumer interface for platform abstraction, you should not call it directly
     * @see InputEventConsumer - Platform integration interface
     */
    void report_axis_change(Axis axis, glm::vec2 value) override;

    /**
     * Requests a change to the mouse cursor mode (visibility and lock state).
     *
     * Instead of calling platform APIs directly, this method creates a SetMouseCursorEvent and
     * dispatches it through the event system. The Window handles this event and applies the
     * mode change via platform-specific calls (e.g., glfwSetInputMode for GLFW).
     *
     * This event-based approach maintains the separation between input abstraction and platform
     * implementation, allowing the InputManager to request cursor changes without depending on
     * the Window implementation.
     *
     * Cursor modes:
     * - CursorMode::Normal - Cursor visible and free-moving (default, for UI navigation)
     * - CursorMode::Hidden - Cursor invisible but still free-moving (for custom cursors)
     * - CursorMode::Locked - Cursor invisible and confined to window (for FPS camera control)
     *
     * @param mode The desired cursor mode
     * @see CursorMode - Enumeration of available cursor modes
     * @see SetMouseCursorEvent - Event dispatched to request cursor mode change
     */
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
} // namespace portal
