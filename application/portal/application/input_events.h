//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include <cstdint>

namespace portal
{
enum class EventSource
{
    Keyboard,
    Mouse,
    Touchscreen
};

class InputEvent
{
public:
    explicit InputEvent(const EventSource source): source(source)
    {}

    [[nodiscard]] EventSource get_source() const { return source; }

private:
    EventSource source;
};

enum class KeyCode
{
    Unknown,
    Space,
    Apostrophe, /* ' */
    Comma,      /* , */
    Minus,      /* - */
    Period,     /* . */
    Slash,      /* / */
    _0,
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,
    Semicolon, /* ; */
    Equal,     /* = */
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
    LeftBracket,  /* [ */
    Backslash,    /* \ */
    RightBracket, /* ] */
    GraveAccent,  /* ` */
    Escape,
    Enter,
    Tab,
    Backspace,
    Insert,
    DelKey,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    Back,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
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
    KP_0,
    KP_1,
    KP_2,
    KP_3,
    KP_4,
    KP_5,
    KP_6,
    KP_7,
    KP_8,
    KP_9,
    KP_Decimal,
    KP_Divide,
    KP_Multiply,
    KP_Subtract,
    KP_Add,
    KP_Enter,
    KP_Equal,
    LeftShift,
    LeftControl,
    LeftAlt,
    RightShift,
    RightControl,
    RightAlt
};

enum class KeyAction
{
    Down,
    Up,
    Repeat,
    Unknown
};

class KeyInputEvent : public InputEvent
{
public:
    KeyInputEvent(const KeyCode code, const KeyAction action): InputEvent(EventSource::Keyboard),
                                                               code(code),
                                                               action(action)
    {}

    [[nodiscard]] KeyCode get_code() const { return code; }
    [[nodiscard]] KeyAction get_action() const { return action; }

private:
    KeyCode code;
    KeyAction action;
};

enum class MouseButton
{
    Left,
    Right,
    Middle,
    Back,
    Forward,
    Unknown
};

enum class MouseAction
{
    Down,
    Up,
    Move,
    Unknown
};

class MouseButtonInputEvent : public InputEvent
{
public:
    MouseButtonInputEvent(const MouseButton button, const MouseAction action, const float pos_x, const float pos_y):
        InputEvent(EventSource::Mouse),
        button(button),
        action(action),
        pos_x(pos_x),
        pos_y(pos_y)
    {}

    [[nodiscard]] MouseButton get_button() const { return button; }
    [[nodiscard]] MouseAction get_action() const { return action; }

    [[nodiscard]] float get_pos_x() const { return pos_x; }
    [[nodiscard]] float get_pos_y() const { return pos_y; }

private:
    MouseButton button;
    MouseAction action;

    float pos_x;
    float pos_y;
};

enum class TouchAction
{
    Down,
    Up,
    Move,
    Cancel,
    PointerDown,
    PointerUp,
    Unknown
};

class TouchInputEvent : public InputEvent
{
public:
    TouchInputEvent(
        const int32_t pointer_id,
        const size_t pointer_count,
        const TouchAction action,
        const float pos_x,
        const float pos_y
    ):
        InputEvent(EventSource::Touchscreen),
        action(action),
        pointer_id(pointer_id),
        touch_points(pointer_count),
        pos_x(pos_x),
        pos_y(pos_y)
    {}

    [[nodiscard]] TouchAction get_action() const;
    [[nodiscard]] int32_t get_pointer_id() const;
    [[nodiscard]] size_t get_touch_points() const;

    [[nodiscard]] float get_pos_x() const;
    [[nodiscard]] float get_pos_y() const;

private:
    TouchAction action;
    int32_t pointer_id;
    size_t touch_points;

    float pos_x;
    float pos_y;
};
}
