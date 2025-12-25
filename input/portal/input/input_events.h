//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "input_types.h"
#include "portal/core/events/event.h"


namespace portal
{
/**
 * Event fired when a key transitions to the Pressed state.
 *
 * Triggered exactly once when a keyboard key or mouse button is initially pressed down
 * (state transition from Released to Pressed). Contains the key identifier and active
 * modifier flags, enabling keyboard shortcut detection (e.g., Ctrl+S).
 *
 * After one frame, the key automatically transitions to Repeat state and subsequent
 * events will be KeyRepeatEvent instead.
 *
 * @see KeyRepeatEvent - Fired while key is held down
 * @see KeyReleasedEvent - Fired when key is released
 */
class KeyPressedEvent final : public Event
{
public:
    KeyPressedEvent(const Key key, const KeyModifierFlag modifiers) :
        key(key), active_modifiers(modifiers)
    {}

    [[nodiscard]] Key get_key() const { return key; }
    [[nodiscard]] KeyModifierFlag get_modifiers() const { return active_modifiers; }

    EVENT_CLASS_TYPE(KeyPressed)
    EVENT_CLASS_CATEGORY(Input)

private:
    Key key;
    KeyModifierFlag active_modifiers;
};

/**
 * Event fired when a key transitions to the Released state.
 *
 * Triggered when a keyboard key or mouse button is released (state transition from
 * Pressed or Repeat to Released). Does not include modifier information since the
 * action is a release rather than an actuation.
 *
 * @see KeyPressedEvent - Fired when key is initially pressed
 */
class KeyReleasedEvent final : public Event
{
public:
    explicit KeyReleasedEvent(const Key key) :
        key(key)
    {}

    [[nodiscard]] Key get_key() const { return key; }

    EVENT_CLASS_TYPE(KeyReleased)
    EVENT_CLASS_CATEGORY(Input)

private:
    Key key;
};

/**
 * Event fired while a key is held in the Repeat state.
 *
 * Triggered after a key has been held down past its initial Pressed frame. The key
 * automatically transitions from Pressed to Repeat after one frame, and this event
 * continues to fire while the key is held. Contains the key identifier and active
 * modifier flags.
 *
 * @see KeyPressedEvent - Fired when key is initially pressed
 */
class KeyRepeatEvent final : public Event
{
public:
    explicit KeyRepeatEvent(const Key key, const KeyModifierFlag modifiers) :
        key(key), active_modifiers(modifiers)
    {}

    [[nodiscard]] Key get_key() const { return key; }
    [[nodiscard]] KeyModifierFlag get_modifiers() const { return active_modifiers; }

    EVENT_CLASS_TYPE(KeyRepeat)
    EVENT_CLASS_CATEGORY(Input)

private:
    Key key;
    KeyModifierFlag active_modifiers;
};


/**
 * Event fired when the mouse cursor moves within the window.
 *
 * Contains the new absolute position in window-space coordinates (origin typically at
 * top-left corner). Fired continuously as the mouse moves, even when the cursor is
 * locked (CursorMode::Locked).
 *
 * @see InputManager::get_mouse_position() - Query cached position
 */
class MouseMovedEvent final : public Event
{
public:
    explicit MouseMovedEvent(glm::vec2 pos) :
        position(pos)
    {}

    [[nodiscard]] float get_x() const { return position.x; }
    [[nodiscard]] float get_y() const { return position.y; }
    [[nodiscard]] glm::vec2 get_position() const { return position; }

    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(Input)

private:
    glm::vec2 position;
};

/**
 * Event fired when the mouse scroll wheel rotates.
 *
 * Contains scroll offset as a 2D vector: y component for vertical scrolling (positive = up,
 * negative = down), x component for horizontal scrolling on supported devices (trackpads,
 * horizontal scroll wheels).
 *
 * @see InputManager::get_mouse_scroll() - Query cached scroll offset
 */
class MouseScrolledEvent final : public Event
{
public:
    explicit MouseScrolledEvent(glm::vec2 offset) :
        offset(offset)
    {}

    [[nodiscard]] float get_x_offset() const { return offset.x; }
    [[nodiscard]] float get_y_offset() const { return offset.y; }
    [[nodiscard]] glm::vec2 get_offset() const { return offset; }

    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(Input)

private:
    glm::vec2 offset;
};

/**
 * Event dispatched to request a cursor mode change.
 *
 * Created by InputManager::set_cursor_mode() and handled by the Window, which applies
 * the mode change via platform-specific calls. This event-based approach maintains
 * separation between input abstraction and platform implementation.
 *
 * @see InputManager::set_cursor_mode() - Dispatches this event
 * @see CursorMode - Available cursor modes
 */
class SetMouseCursorEvent final : public Event
{
public:
    explicit SetMouseCursorEvent(const CursorMode mode) :
        mode(mode)
    {}
    [[nodiscard]] CursorMode get_mode() const { return mode; }

    EVENT_CLASS_TYPE(SetMouseCursor)
    EVENT_CLASS_CATEGORY(Input)

private:
    CursorMode mode;
};
} // namespace portal
