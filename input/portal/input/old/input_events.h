//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "input_types.h"


namespace portal
{

struct ReportKeyActionEvent
{
    Key key{};
    KeyState state{};
    std::optional<KeyModifierFlag> modifiers;
};

struct ReportAnalogAxisEvent
{
    Axis axis;
    glm::vec2 value;
};

/**
 * Fired once when a key is initially pressed (Released → Pressed).
 * Includes modifiers for shortcut detection (e.g., Ctrl+S).
 */
struct KeyPressedEvent
{
    Key key;
    KeyModifierFlag modifiers;
};

/**
 * Fired when a key is released (Pressed/Repeat → Released).
 */
struct KeyReleasedEvent
{
    Key key;
};

/**
 * Fired while a key is held down (Repeat state after initial press).
 */
struct KeyRepeatEvent
{
    Key key;
    KeyModifierFlag active_modifiers;
};


/**
 * Fired when the mouse moves. Contains absolute window-space position.
 */
struct MouseMovedEvent
{
    glm::vec2 position;
};

/**
 * Fired on scroll wheel rotation. Contains offset (x = horizontal, y = vertical).
 */
struct MouseScrolledEvent
{
    glm::vec2 offset;
};

/**
 * Requests cursor mode change. Dispatched by InputManager, handled by Window.
 */
struct SetMouseCursorEvent
{
    CursorMode mode;
};
} // namespace portal
