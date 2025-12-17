//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/flags.h"

namespace portal
{
enum class Key: uint16_t
{
    Invalid = 0,
    /// ----- Mouse -----
    MouseButton0,
    MouseButton1,
    MouseButton2,
    MouseButton3,
    MouseButton4,
    MouseButton5,
    LeftMouseButton   = MouseButton0,
    RightMouseButton  = MouseButton1,
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

// TODO: integrate to keys
enum class Axis
{
    Mouse,
    MouseScroll
};

enum class KeyState
{
    Pressed,
    Released,
    Repeat,
};

enum class CursorMode
{
    Normal,
    Hidden,
    Locked
};

enum class KeyModifierBits: uint8_t
{
    None     = 0b00000000,
    Shift    = 0b00000001,
    Ctrl     = 0b00000010,
    Alt      = 0b00000100,
    System   = 0b00001000,
    CapsLock = 0b00010000,
    NumLock  = 0b00100000,
};

using KeyModifierFlag = Flags<KeyModifierBits>;

template <>
struct FlagTraits<KeyModifierBits>
{
    using enum KeyModifierBits;

    static constexpr bool is_bitmask = true;
    static constexpr Flags<KeyModifierBits> all_flags = Shift | Ctrl | Alt | System | CapsLock | NumLock;
};
}
