//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal
{
enum class Key
{
    AnyKey,

    MouseX,
    MouseY,
    Mouse2D,
    MouseScrollUp,
    MouseScrollDown,
    MouseWheelAxis,

    LeftMouseButton,
    RightMouseButton,
    MiddleMouseButton,
    ThumbMouseButton,
    ThumbMouseButton2,

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

    Left,
    Up,
    Right,
    Down,

    Insert,
    Delete,

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

    NumPadZero,
    NumPadOne,
    NumPadTwo,
    NumPadThree,
    NumPadFour,
    NumPadFive,
    NumPadSix,
    NumPadSeven,
    NumPadEight,
    NumPadNine,

    Multiply,
    Add,
    Subtract,
    Decimal,
    Divide,

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

    NumLock,

    ScrollLock,

    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,
    LeftCommand,
    RightCommand,

    Semicolon,
    Equals,
    Comma,
    Underscore,
    Hyphen,
    Period,
    Slash,
    Tilde,
    LeftBracket,
    Backslash,
    RightBracket,
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

    A_AccentGrave,
    E_AccentGrave,
    E_AccentAigu,
    C_Cedille,
    Section,

    // Platform Keys
    // These keys platform specific versions of keys that go by different names.
    // The delete key is a good example, on Windows Delete is the virtual key for Delete.
    // On Macs, the Delete key is the virtual key for BackSpace.
    Platform_Delete,

    // Gamepad Keys
    Gamepad_Left2D,
    Gamepad_LeftX,
    Gamepad_LeftY,
    Gamepad_Right2D,
    Gamepad_RightX,
    Gamepad_RightY,
    Gamepad_LeftTriggerAxis,
    Gamepad_RightTriggerAxis,

    Gamepad_LeftThumbstick,
    Gamepad_RightThumbstick,
    Gamepad_Special_Left,
    Gamepad_Special_Left_X,
    Gamepad_Special_Left_Y,
    Gamepad_Special_Left_Touched,
    Gamepad_Special_Right,
    Gamepad_FaceButton_Bottom,
    Gamepad_FaceButton_Right,
    Gamepad_FaceButton_Left,
    Gamepad_FaceButton_Top,
    Gamepad_LeftShoulder,
    Gamepad_RightShoulder,
    Gamepad_LeftTrigger,
    Gamepad_RightTrigger,
    Gamepad_DPad_Up,
    Gamepad_DPad_Down,
    Gamepad_DPad_Right,
    Gamepad_DPad_Left,

    // Virtual key codes used for input axis button press/release emulation
    Gamepad_LeftStick_Up,
    Gamepad_LeftStick_Down,
    Gamepad_LeftStick_Right,
    Gamepad_LeftStick_Left,

    Gamepad_RightStick_Up,
    Gamepad_RightStick_Down,
    Gamepad_RightStick_Right,
    Gamepad_RightStick_Left,

    // Virtual buttons that use other buttons depending on the platform
    Virtual_Gamepad_Accept,
    Virtual_Gamepad_Back,

    Invalid
};
}
