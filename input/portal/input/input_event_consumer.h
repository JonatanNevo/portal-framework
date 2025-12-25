//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <optional>
#include <portal/core/glm.h>
#include "portal/input/input_types.h"

namespace portal
{
/**
 * Type erasure interface for reporting input events to the input system.
 *
 * InputEventConsumer defines the boundary between platform-specific input code (GLFW, SDL, etc.)
 * and Portal's platform-agnostic InputManager. Platform layers translate native input codes to
 * Portal's Key/Axis enums and report events through this interface, keeping the InputManager
 * independent of any specific platform API.
 *
 * The InputManager implements this interface and receives callbacks from the platform layer
 * (e.g., GlfwWindow GLFW callbacks) when hardware input occurs.
 *
 * @see InputManager - Implements this interface to receive platform input
 * @see Key - Unified key/button enumeration
 * @see Axis - Analog input axis enumeration
 */
class InputEventConsumer
{
public:
    virtual ~InputEventConsumer() = default;

    /**
     * Reports a keyboard or mouse button state change from the platform layer.
     *
     * Called by platform implementations when a key or mouse button changes state. The platform
     * must translate its native codes (e.g., GLFW_KEY_*, GLFW_MOUSE_BUTTON_*) to Portal's Key
     * enum before calling this method.
     *
     * @param key The key that changed (already translated from platform-specific codes)
     * @param state The new state (Pressed, Released, or Repeat)
     * @param modifiers Optional bitflags for active modifiers (Shift, Ctrl, Alt, etc.)
     */
    virtual void report_key_action(Key key, KeyState state, std::optional<KeyModifierFlag> modifiers);

    /**
     * Reports analog input changes from the platform layer (mouse movement, scroll).
     *
     * Called by platform implementations when analog input values change, such as mouse cursor
     * position or scroll wheel rotation.
     *
     * @param axis The axis that changed (Mouse for cursor movement, MouseScroll for scroll wheel)
     * @param value For Mouse: absolute window-space position (x, y). For MouseScroll: scroll
     *              offset (x = horizontal, y = vertical)
     */
    virtual void report_axis_change(Axis axis, glm::vec2 value);
};
} // namespace portal
