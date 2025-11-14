//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/flags.h"

namespace portal::ng
{

enum class ConsoleType
{
    None,
    Xbox,
    PlayStation
};

enum class PairedAxis : uint8_t
{
    Unpaired,
    X,
    Y,
    Z,
};

enum class KeyFlagsBits: uint16_t
{
    GamepadKey               = 0b0000000000000001,
    Touch                    = 0b0000000000000010,
    MouseButton              = 0b0000000000000100,
    ModifierKey              = 0b0000000000001000,
    Axis1D                   = 0b0000000000010000,
    Axis2D                   = 0b0000000000100000,
    Axis3D                   = 0b0000000001000000,
    UpdateAxisWithoutSamples = 0b0000000010000000,
    ButtonAxis               = 0b0000000100000000, // Emulates a 1D axis with a digital button

    // A key is "Virtual" if it is an abstract key whose actual value may change dependent on the platform.
    // For example, the standard "Accept" button on some platforms may be Gamepad_FaceButton_Down, while on
    // other platforms it may be Gamepad_FaceButton_Right.
    //
    // Virtual keys are typically the most useful when it comes to building UI related code because they allow you
    // to create experiences and screens displayed to the end user which are consistent with the platform's UI.
    Virtual = 0b0000010000000000,
    Empty   = 0
};

using KeyFlags = Flags<KeyFlagsBits>;


enum class Key: uint16_t
{
    Invalid = 0,
    /// ----- Mouse -----
    // Axis
    MouseX,
    MouseY,
    Mouse2D,
    MouseScrollUp,
    MouseScrollDown,
    MouseWheelAxis,

    // Keys
    LeftMouseButton,
    RightMouseButton,
    MiddleMouseButton,
    ThumbMouseButton,
    ThumbMouseButton2,

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
    LeftSystem,     // Windows Key in Windows, Command in Mac
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

    /// ----- Gamepad -----
    // Axis
    GamepadLeftX,
    GamepadLeftY,
    GamepadLeft2D,
    GamepadRightX,
    GamepadRightY,
    GamepadRight2D,
    GamepadLeftTriggerAxis,
    GamepadRightTriggerAxis,

    // Keys
    GamepadLeftThumbstick,
    GamepadRightThumbstick,
    GamepadLeftShoulder, // TODO: Some newer controller have the shoulder button as an analog axis as well
    GamepadRightShoulder,
    GamepadLeftTrigger,
    GamepadRightTrigger,
    GamepadFaceRight,
    GamepadFaceLeft,
    GamepadFaceUp,
    GamepadFaceDown,
    GamepadDPadRight,
    GamepadDPadLeft,
    GamepadDPadUp,
    GamepadDPadDown,
    GamepadSpecialLeft,
    GamepadSpecialRight,

    /// ----- Virtual Keys -----

    // Gamepad
    GamepadLeftStickLeft,
    GamepadLeftStickRight,
    GamepadLeftStickUp,
    GamepadLeftStickDown,
    GamepadRightStickLeft,
    GamepadRightStickRight,
    GamepadRightStickUp,
    GamepadRightStickDown,
    GamepadVirtualAccept,
    GamepadVirtualBack,

    Any = std::numeric_limits<std::underlying_type_t<Key>>::max()
};

enum class InputEventType: uint8_t
{
    Pressed,
    Released,
    Repeat,
    DoubleClick,
    Axis
};

}

template <>
struct ::portal::FlagTraits<::portal::ng::KeyFlagsBits>
{
    using enum ng::KeyFlagsBits;

    static constexpr bool is_bitmask = true;
    static constexpr Flags<ng::KeyFlagsBits> all_flags = GamepadKey | Touch | MouseButton | ModifierKey | Axis1D | Axis2D | Axis3D |
        UpdateAxisWithoutSamples | ButtonAxis | Virtual;
};

// template <>
// struct std::hash<portal::ng::Key>
// {
//     size_t operator()(const portal::ng::Key& key) const noexcept
//     {
//         return std::to_underlying(key);
//     }
// };
//
// template <>
// struct frozen::elsa<portal::ng::Key>
// {
//     constexpr std::size_t operator()(const portal::ng::Key& value, const std::size_t seed) const
//     {
//         return seed ^ std::to_underlying(value);
//     }
// };
