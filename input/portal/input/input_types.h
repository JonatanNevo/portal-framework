//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/flags.h"

namespace portal
{

/**
 * Unified enumeration representing physical keyboard keys and mouse buttons.
 *
 * The Key enum provides a cross-platform abstraction for all digital input (buttons that can be
 * pressed or released). It unifies keyboard keys and mouse buttons into a single enumeration,
 * simplifying state tracking and allowing the same input handling code to work with both
 * keyboard and mouse input.
 *
 * ## Physical Layout Mapping
 *
 * Keys are mapped to physical key locations following the US QWERTY keyboard layout, not character
 * codes. This ensures consistent behavior across different keyboard layouts and languages. For example,
 * Key::W always refers to the physical key in the top-left letter position (typically labeled "W" on
 * US keyboards), regardless of the user's system language or keyboard layout settings.
 *
 * ## Key Categories
 *
 * - **Mouse**: MouseButton0-5 with LeftMouseButton, RightMouseButton, MiddleMouseButton aliases
 * - **Letters**: A-Z (physical positions, not character codes)
 * - **Numbers**: Zero-Nine (top row), NumpadZero-NumpadNine (numpad)
 * - **Modifiers**: LeftShift, RightShift, LeftControl, RightControl, LeftAlt, RightAlt,
 *                  LeftSystem/RightSystem (Windows key on Windows, Command on Mac)
 * - **Control**: BackSpace, Tab, Enter, Escape, SpaceBar, arrows, function keys F1-F12
 * - **Special**: Semicolon, Equals, Comma, brackets, slash, etc.
 *
 * ## Special Values
 *
 * - Key::Invalid (value 0) - Represents an unmapped or unknown key
 * - Key::Max - Marks the end of valid key values (used for iteration bounds)
 * - Key::Any - Wildcard value for matching any key (max value of underlying type)
 *
 * ## Future Expansion
 *
 * The enum currently supports keyboard and mouse buttons. Gamepad/controller support is planned.

 * @see KeyState - Enumeration of key states (Pressed, Released, Repeat)
 * @see InputManager - Tracks state for all keys using this enum
 */
enum class Key : uint16_t
{
    Invalid = 0,
    /// ----- Mouse -----
    MouseButton0,
    MouseButton1,
    MouseButton2,
    MouseButton3,
    MouseButton4,
    MouseButton5,
    LeftMouseButton = MouseButton0,
    RightMouseButton = MouseButton1,
    MiddleMouseButton = MouseButton2,

    /// ----- Letters -----
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    /// ----- Numbers -----
    Zero,
    One,
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    NumpadZero,
    NumpadOne,
    NumpadTwo,
    NumpadThree,
    NumpadFour,
    NumpadFive,
    NumpadSix,
    NumpadSeven,
    NumpadEight,
    NumpadNine,

    // Numerical Operations
    Multiply,
    Add,
    Subtract,
    Decimal,
    Divide,

    /// ----- Modifier Keys -----
    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,
    LeftSystem, // Windows Key in Windows, Command in Mac
    RightSystem,

    /// ----- Control Keys -----
    BackSpace,
    Tab,
    Enter,
    Pause,
    CapsLock,
    Escape,
    SpaceBar,
    PageUp,
    PageDown,
    End,
    Home,
    Insert,
    Delete,
    NumLock,
    ScrollLock,

    // Arrows
    Left,
    Right,
    Up,
    Down,

    // Function keys
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    /// ----- Special Characters -----
    Semicolon,
    Equals,
    Comma,
    Underscore,
    Hyphen,
    Period,
    Slash,
    Tilde,
    LeftBracket,
    RightBracket,
    Backslash,
    Apostrophe,
    Ampersand,
    Asterix,
    Caret,
    Colon,
    Dollar,
    Exclamation,
    LeftParantheses,
    RightParantheses,
    Quote,

    // TODO: Add controller support

    Max,
    Any = std::numeric_limits<std::underlying_type_t<Key>>::max()
};

/**
 * Enumeration for analog (continuous) input axes.
 *
 * Axes represent continuous input values rather than digital button presses. They provide
 * 2D vector data for input devices that report position or delta values, such as mouse
 * cursor movement, scroll wheel rotation, or (in the future) gamepad analog sticks.
 *
 * ## Current Axes
 *
 * - **Mouse** - Mouse cursor movement, reported as absolute window-space position (x, y)
 * - **MouseScroll** - Scroll wheel rotation, reported as delta offset (x = horizontal, y = vertical)
 *
 * @see InputEventConsumer::report_axis_change() - Platform layer reports axis changes
 * @see InputManager::get_mouse_position() - Query cached mouse position
 * @see InputManager::get_mouse_scroll() - Query cached scroll offset
 */
// TODO: integrate to keys
enum class Axis
{
    Mouse,
    MouseScroll
};

/**
 * State of a key during its lifecycle.
 *
 * Keys transition: Released → Pressed (one frame) → Repeat (held) → Released.
 * The Pressed→Repeat transition happens automatically via transition_key_states().
 *
 * Usage: is_key_pressed() returns true for both Pressed and Repeat (meaning "down").
 * To detect initial press only, check: is_key_pressed() && !is_key_repeating().
 */
enum class KeyState
{
    Pressed,
    Released,
    Repeat,
};

/**
 * Mouse cursor visibility and movement behavior.
 *
 * Set via InputManager::set_cursor_mode() which dispatches SetMouseCursorEvent
 * to the Window for platform-specific handling.
 */
enum class CursorMode
{
    Normal,
    Hidden,
    Locked
};

/**
 * Bit flags for modifier keys active during input events.
 *
 * Used with Flags<KeyModifierBits> for type-safe bitflag operations.
 * Check modifiers in events: modifiers.has(KeyModifierBits::Ctrl)
 *
 * @see KeyModifierFlag - Type alias for Flags<KeyModifierBits>
 */
enum class KeyModifierBits : uint8_t
{
    None = 0b00000000,
    Shift = 0b00000001,
    Ctrl = 0b00000010,
    Alt = 0b00000100,
    System = 0b00001000,
    CapsLock = 0b00010000,
    NumLock = 0b00100000,
};

using KeyModifierFlag = Flags<KeyModifierBits>;

template <>
struct FlagTraits<KeyModifierBits>
{
    using enum KeyModifierBits;

    static constexpr bool is_bitmask = true;
    static constexpr Flags<KeyModifierBits> all_flags = Shift | Ctrl | Alt | System | CapsLock | NumLock;
};
} // namespace portal
