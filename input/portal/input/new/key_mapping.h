//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once


#include "portal/core/debug/assert.h"
#include "portal/core/strings/string_id.h"
#include "portal/input/new/key_details.h"

namespace portal::ng
{

namespace details
{
    constexpr static StringId get_console_display_name(const ConsoleType type, const Key key)
    {
        switch (type)
        {
            using enum Key;
            using enum ConsoleType;
        case PlayStation:
            if (key == GamepadFaceLeft)
                return STRING_ID("Gamepad Square");
            if (key == GamepadFaceRight)
                return STRING_ID("Gamepad Circle");
            if (key == GamepadFaceUp)
                return STRING_ID("Gamepad Triangle");
            if (key == GamepadFaceDown)
                return STRING_ID("Gamepad Cross");
            if (key == GamepadSpecialLeft)
                return STRING_ID("Gamepad Touch Button");
            if (key == GamepadSpecialRight)
                return STRING_ID("Gamepad Options");
            if (key == GamepadLeftShoulder)
                return STRING_ID("Gamepad L1");
            if (key == GamepadRightShoulder)
                return STRING_ID("Gamepad R1");
            if (key == GamepadLeftTrigger)
                return STRING_ID("Gamepad L2");
            if (key == GamepadRightTrigger)
                return STRING_ID("Gamepad R2");
            if (key == GamepadLeftTriggerAxis)
                return STRING_ID("Gamepad L2 Axis");
            if (key == GamepadRightTriggerAxis)
                return STRING_ID("Gamepad R2 Axis");
            if (key == GamepadLeftThumbstick)
                return STRING_ID("Gamepad L3");
            if (key == GamepadRightThumbstick)
                return STRING_ID("Gamepad R3");
            break;
        case Xbox:
            if (key == GamepadFaceLeft)
                return STRING_ID("Gamepad X");
            if (key == GamepadFaceRight)
                return STRING_ID("Gamepad B");
            if (key == GamepadFaceUp)
                return STRING_ID("Gamepad Y");
            if (key == GamepadFaceDown)
                return STRING_ID("Gamepad A");
            if (key == GamepadSpecialLeft)
                return STRING_ID("Gamepad Back");
            if (key == GamepadSpecialRight)
                return STRING_ID("Gamepad Start");
            if (key == GamepadLeftShoulder)
                return STRING_ID("Gamepad Left Shoulder");
            if (key == GamepadRightShoulder)
                return STRING_ID("Gamepad Right shoulder");
            if (key == GamepadLeftTrigger)
                return STRING_ID("Gamepad Left Trigger");
            if (key == GamepadRightTrigger)
                return STRING_ID("Gamepad Right Trigger");
            if (key == GamepadLeftTriggerAxis)
                return STRING_ID("Gamepad Left Trigger Axis");
            if (key == GamepadRightTriggerAxis)
                return STRING_ID("Gamepad Right Trigger Axis");
            if (key == GamepadLeftThumbstick)
                return STRING_ID("Gamepad Left Thumbstick Button");
            if (key == GamepadRightThumbstick)
                return STRING_ID("Gamepad Right Thumbstick Button");
            break;
        default:
            if (key == GamepadFaceLeft)
                return STRING_ID("Gamepad Face Button Left");
            if (key == GamepadFaceRight)
                return STRING_ID("Gamepad Face Button Right");
            if (key == GamepadFaceUp)
                return STRING_ID("Gamepad Face Button Up");
            if (key == GamepadFaceDown)
                return STRING_ID("Gamepad Face Button Down");
            if (key == GamepadSpecialLeft)
                return STRING_ID("Gamepad Special Left");
            if (key == GamepadSpecialRight)
                return STRING_ID("Gamepad Special Right");
            if (key == GamepadLeftShoulder)
                return STRING_ID("Gamepad Left Shoulder");
            if (key == GamepadRightShoulder)
                return STRING_ID("Gamepad Right shoulder");
            if (key == GamepadLeftTrigger)
                return STRING_ID("Gamepad Left Trigger");
            if (key == GamepadRightTrigger)
                return STRING_ID("Gamepad Right Trigger");
            if (key == GamepadLeftTriggerAxis)
                return STRING_ID("Gamepad Left Trigger Axis");
            if (key == GamepadRightTriggerAxis)
                return STRING_ID("Gamepad Right Trigger Axis");
            if (key == GamepadLeftThumbstick)
                return STRING_ID("Gamepad Left Thumbstick Button");
            if (key == GamepadRightThumbstick)
                return STRING_ID("Gamepad Right Thumbstick Button");
            break;
        }

        PORTAL_ASSERT(false, "Unexpected key");
        return INVALID_STRING_ID;
    }
}


class KeyMapping
{
public:
    constinit static frozen::unordered_map<Key, KeyDetails, 155> key_mapping;

    constexpr static const KeyDetails& get_details(const Key key)
    {
        const auto it = key_mapping.find(key);
        if (it == key_mapping.end())
            return key_mapping.at(Key::Invalid);
        return it->second;
    }

private:
    constexpr static auto make_key_mapping(const ConsoleType type)
    {
        using enum Key;
        using enum KeyFlagsBits;

#define ADD_KEY(key, ...) {key, KeyDetails{key, __VA_ARGS__}}
#define ADD_PAIRED_KEY(key, axis, parent, ...) {key, KeyDetails{key, __VA_ARGS__, axis, parent}}
        // TODO: Create this dynamically based on connected gamepad
        return frozen::unordered_map<Key, KeyDetails, 155>{
            ADD_KEY(Invalid, STRING_ID("Unknown Key")),
            ADD_KEY(Any, STRING_ID("Any Key")),

            ADD_PAIRED_KEY(MouseX, PairedAxis::X, Mouse2D, STRING_ID("Mouse X"), Axis1D | MouseButton | UpdateAxisWithoutSamples),
            ADD_PAIRED_KEY(MouseY, PairedAxis::Y, Mouse2D, STRING_ID("Mouse Y"), Axis1D | MouseButton | UpdateAxisWithoutSamples),
            ADD_KEY(Mouse2D, STRING_ID("Mouse XY 2D-Axis"), Axis2D | MouseButton | UpdateAxisWithoutSamples),
            ADD_KEY(MouseWheelAxis, STRING_ID("Mouse Wheel Axis"), Axis1D | MouseButton | UpdateAxisWithoutSamples),
            ADD_KEY(MouseScrollUp, STRING_ID("Mouse Scroll Up"), MouseButton | ButtonAxis),
            ADD_KEY(MouseScrollDown, STRING_ID("Mouse Scroll Down"), MouseButton | ButtonAxis),

            ADD_KEY(LeftMouseButton, STRING_ID("Left Mouse Button"), MouseButton, STRING_ID("LMB")),
            ADD_KEY(RightMouseButton, STRING_ID("Right Mouse Button"), MouseButton, STRING_ID("RMB")),
            ADD_KEY(MiddleMouseButton, STRING_ID("Middle Mouse Button"), MouseButton, STRING_ID("MMB")),
            ADD_KEY(ThumbMouseButton, STRING_ID("Thumb Mouse Button"), MouseButton, STRING_ID("MB4")),
            ADD_KEY(ThumbMouseButton2, STRING_ID("Thumb Mouse Button2"), MouseButton, STRING_ID("MB5")),

            ADD_KEY(A, STRING_ID("A")),
            ADD_KEY(B, STRING_ID("B")),
            ADD_KEY(C, STRING_ID("C")),
            ADD_KEY(D, STRING_ID("D")),
            ADD_KEY(E, STRING_ID("E")),
            ADD_KEY(F, STRING_ID("F")),
            ADD_KEY(G, STRING_ID("G")),
            ADD_KEY(H, STRING_ID("H")),
            ADD_KEY(I, STRING_ID("I")),
            ADD_KEY(J, STRING_ID("J")),
            ADD_KEY(K, STRING_ID("K")),
            ADD_KEY(L, STRING_ID("L")),
            ADD_KEY(M, STRING_ID("M")),
            ADD_KEY(N, STRING_ID("N")),
            ADD_KEY(O, STRING_ID("O")),
            ADD_KEY(P, STRING_ID("P")),
            ADD_KEY(Q, STRING_ID("Q")),
            ADD_KEY(R, STRING_ID("R")),
            ADD_KEY(S, STRING_ID("S")),
            ADD_KEY(T, STRING_ID("T")),
            ADD_KEY(U, STRING_ID("U")),
            ADD_KEY(V, STRING_ID("V")),
            ADD_KEY(W, STRING_ID("W")),
            ADD_KEY(X, STRING_ID("X")),
            ADD_KEY(Y, STRING_ID("Y")),
            ADD_KEY(Z, STRING_ID("Z")),

            ADD_KEY(Zero, STRING_ID("0")),
            ADD_KEY(One, STRING_ID("1")),
            ADD_KEY(Two, STRING_ID("2")),
            ADD_KEY(Three, STRING_ID("3")),
            ADD_KEY(Four, STRING_ID("4")),
            ADD_KEY(Five, STRING_ID("5")),
            ADD_KEY(Six, STRING_ID("6")),
            ADD_KEY(Seven, STRING_ID("7")),
            ADD_KEY(Eight, STRING_ID("8")),
            ADD_KEY(Nine, STRING_ID("9")),
            ADD_KEY(NumpadZero, STRING_ID( "Num 0")),
            ADD_KEY(NumpadOne, STRING_ID( "Num 1")),
            ADD_KEY(NumpadTwo, STRING_ID( "Num 2")),
            ADD_KEY(NumpadThree, STRING_ID( "Num 3")),
            ADD_KEY(NumpadFour, STRING_ID( "Num 4")),
            ADD_KEY(NumpadFive, STRING_ID( "Num 5")),
            ADD_KEY(NumpadSix, STRING_ID( "Num 6")),
            ADD_KEY(NumpadSeven, STRING_ID( "Num 7")),
            ADD_KEY(NumpadEight, STRING_ID( "Num 8")),
            ADD_KEY(NumpadNine, STRING_ID( "Num 9")),

            ADD_KEY(Multiply, STRING_ID("Num *")),
            ADD_KEY(Add, STRING_ID("Num +")),
            ADD_KEY(Subtract, STRING_ID("Num -")),
            ADD_KEY(Decimal, STRING_ID( "Num .")),
            ADD_KEY(Divide, STRING_ID( "Num /")),

            ADD_KEY(LeftShift, STRING_ID("Left Shift"), ModifierKey),
            ADD_KEY(RightShift, STRING_ID("Right Shift"), ModifierKey),
            ADD_KEY(LeftControl, STRING_ID("Left Ctrl"), ModifierKey),
            ADD_KEY(RightControl, STRING_ID("Right Ctrl"), ModifierKey),
            ADD_KEY(LeftAlt, STRING_ID("Left Alt"), ModifierKey),
            ADD_KEY(RightAlt, STRING_ID("Right Alt"), ModifierKey),
            ADD_KEY(LeftSystem, STRING_ID("Left System"), ModifierKey),
            ADD_KEY(RightSystem, STRING_ID("Right System"), ModifierKey),

#if defined(PORTAL_PLATFORM_MACOS)
            ADD_KEY(BackSpace, STRING_ID("Delete"), {}, STRING_ID("Del")),
                ADD_KEY(Delete, STRING_ID("ForwardDelete"), {}, STRING_ID("Fn+Delete")),
#else
            ADD_KEY(BackSpace, STRING_ID("BackSpace")),
            ADD_KEY(Delete, STRING_ID("Delete"), {}, STRING_ID("Del")),
#endif

            ADD_KEY(Tab, STRING_ID("Tab")),
            ADD_KEY(Enter, STRING_ID("Enter")),
            ADD_KEY(Pause, STRING_ID("Pause")),
            ADD_KEY(CapsLock, STRING_ID("Caps Lock"), {}, STRING_ID("Caps")),
            ADD_KEY(Escape, STRING_ID("Escape"), {}, STRING_ID( "Esc")),
            ADD_KEY(SpaceBar, STRING_ID("Space Bar"), {}, STRING_ID("Space")),
            ADD_KEY(PageUp, STRING_ID("Page Up"), {}, STRING_ID("PgUp")),
            ADD_KEY(PageDown, STRING_ID("Page Down"), {}, STRING_ID("PgDn")),
            ADD_KEY(End, STRING_ID("End")),
            ADD_KEY(Home, STRING_ID("Home")),
            ADD_KEY(Insert, STRING_ID( "Insert"), {}, STRING_ID( "Ins")),
            ADD_KEY(NumLock, STRING_ID( "Num Lock")),
            ADD_KEY(ScrollLock, STRING_ID("Scroll Lock")),

            ADD_KEY(Left, STRING_ID("Left")),
            ADD_KEY(Up, STRING_ID("Up")),
            ADD_KEY(Right, STRING_ID("Right")),
            ADD_KEY(Down, STRING_ID("Down")),

            ADD_KEY(F1, STRING_ID("F1")),
            ADD_KEY(F2, STRING_ID("F2")),
            ADD_KEY(F3, STRING_ID("F3")),
            ADD_KEY(F4, STRING_ID("F4")),
            ADD_KEY(F5, STRING_ID("F5")),
            ADD_KEY(F6, STRING_ID("F6")),
            ADD_KEY(F7, STRING_ID("F7")),
            ADD_KEY(F8, STRING_ID("F8")),
            ADD_KEY(F9, STRING_ID("F9")),
            ADD_KEY(F10, STRING_ID("F10")),
            ADD_KEY(F11, STRING_ID("F11")),
            ADD_KEY(F12, STRING_ID("F12")),

            ADD_KEY(Semicolon, STRING_ID("Semicolon"), {}, STRING_ID(";")),
            ADD_KEY(Equals, STRING_ID("Equals"), {}, STRING_ID("=")),
            ADD_KEY(Comma, STRING_ID("Comma"), {}, STRING_ID(",")),
            ADD_KEY(Hyphen, STRING_ID("Hyphen"), {}, STRING_ID("-")),
            ADD_KEY(Underscore, STRING_ID("Underscore"), {}, STRING_ID("_")),
            ADD_KEY(Period, STRING_ID("Period"), {}, STRING_ID(".")),
            ADD_KEY(Slash, STRING_ID("Slash"), {}, STRING_ID("/")),
            ADD_KEY(Tilde, STRING_ID("Tilde"), {}, STRING_ID("~")),
            ADD_KEY(LeftBracket, STRING_ID("Left Bracket"), {}, STRING_ID("[")),
            ADD_KEY(RightBracket, STRING_ID("Right Bracket"), {}, STRING_ID("]")),
            ADD_KEY(Backslash, STRING_ID("Backslash"), {}, STRING_ID("\\")),
            ADD_KEY(Apostrophe, STRING_ID("Apostrophe"), {}, STRING_ID("'")),
            ADD_KEY(Ampersand, STRING_ID("Ampersand"), {}, STRING_ID("&")),
            ADD_KEY(Asterix, STRING_ID("Asterisk"), {}, STRING_ID("*")),
            ADD_KEY(Caret, STRING_ID("Caret"), {}, STRING_ID("^")),
            ADD_KEY(Colon, STRING_ID("Colon"), {}, STRING_ID(":")),
            ADD_KEY(Dollar, STRING_ID("Dollar"), {}, STRING_ID("$")),
            ADD_KEY(Exclamation, STRING_ID("Exclamation"), {}, STRING_ID("!")),
            ADD_KEY(LeftParantheses, STRING_ID("Left Parantheses"), {}, STRING_ID("(")),
            ADD_KEY(RightParantheses, STRING_ID("Right Parantheses"), {}, STRING_ID(")")),
            ADD_KEY(Quote, STRING_ID("Quote"), {}, STRING_ID("\"")),

            /// Gamepad
            ADD_PAIRED_KEY(GamepadLeftX, PairedAxis::X, GamepadLeft2D, STRING_ID("Gamepad Left Thumbstick X-Axis"), GamepadKey | Axis1D),
            ADD_PAIRED_KEY(GamepadLeftY, PairedAxis::Y, GamepadLeft2D, STRING_ID("Gamepad Left Thumbstick Y-Axis"), GamepadKey | Axis1D),
            ADD_KEY(GamepadLeft2D, STRING_ID("Gamepad Left Thumbstick 2D-Axis"), GamepadKey | Axis2D),
            ADD_PAIRED_KEY(GamepadRightX, PairedAxis::X, GamepadRight2D, STRING_ID("Gamepad Right Thumbstick X-Axis"), GamepadKey | Axis1D),
            ADD_PAIRED_KEY(GamepadRightY, PairedAxis::Y, GamepadRight2D, STRING_ID("Gamepad Right Thumbstick Y-Axis"), GamepadKey | Axis1D),
            ADD_KEY(GamepadRight2D, STRING_ID("Gamepad Right Thumbstick 2D-Axis"), GamepadKey | Axis2D),
            ADD_KEY(GamepadLeftTriggerAxis, details::get_console_display_name(type, GamepadLeftTriggerAxis), GamepadKey | Axis1D),
            ADD_KEY(GamepadRightTriggerAxis, details::get_console_display_name(type, GamepadRightTriggerAxis), GamepadKey | Axis1D),

            ADD_KEY(GamepadLeftThumbstick, details::get_console_display_name(type, GamepadLeftThumbstick), GamepadKey),
            ADD_KEY(GamepadRightThumbstick, details::get_console_display_name(type, GamepadRightThumbstick), GamepadKey),
            ADD_KEY(GamepadLeftShoulder, details::get_console_display_name(type, GamepadLeftShoulder), GamepadKey),
            ADD_KEY(GamepadRightShoulder, details::get_console_display_name(type, GamepadRightShoulder), GamepadKey),
            ADD_KEY(GamepadLeftTrigger, details::get_console_display_name(type, GamepadLeftTrigger), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadRightTrigger, details::get_console_display_name(type, GamepadRightTrigger), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadFaceRight, details::get_console_display_name(type, GamepadFaceRight), GamepadKey),
            ADD_KEY(GamepadFaceLeft, details::get_console_display_name(type, GamepadFaceLeft), GamepadKey),
            ADD_KEY(GamepadFaceUp, details::get_console_display_name(type, GamepadFaceUp), GamepadKey),
            ADD_KEY(GamepadFaceDown, details::get_console_display_name(type, GamepadFaceDown), GamepadKey),
            ADD_KEY(GamepadDPadUp, STRING_ID("Gamepad D-pad Up"), GamepadKey),
            ADD_KEY(GamepadDPadDown, STRING_ID("Gamepad D-pad Down"), GamepadKey),
            ADD_KEY(GamepadDPadRight, STRING_ID("Gamepad D-pad Right"), GamepadKey),
            ADD_KEY(GamepadDPadLeft, STRING_ID("Gamepad D-pad Left"), GamepadKey),
            ADD_KEY(GamepadSpecialLeft, details::get_console_display_name(type, GamepadSpecialLeft), GamepadKey),
            ADD_KEY(GamepadSpecialRight, details::get_console_display_name(type, GamepadSpecialRight), GamepadKey),

            ADD_KEY(GamepadLeftStickUp, STRING_ID("Gamepad Left Thumbstick Up"), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadLeftStickDown, STRING_ID("Gamepad Left Thumbstick Down"), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadLeftStickRight, STRING_ID("Gamepad Left Thumbstick Right"), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadLeftStickLeft, STRING_ID("Gamepad Left Thumbstick Left"), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadRightStickUp, STRING_ID("Gamepad Right Thumbstick Up"), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadRightStickDown, STRING_ID("Gamepad Right Thumbstick Down"), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadRightStickRight, STRING_ID("Gamepad Right Thumbstick Right"), GamepadKey | ButtonAxis),
            ADD_KEY(GamepadRightStickLeft, STRING_ID("Gamepad Right Thumbstick Left"), GamepadKey | ButtonAxis),

            /// Virtual
            // TODO: Create virtual accept keys based on gamepad
            // AddVirtualKey(FKeyDetails(EKeys::Virtual_Gamepad_Accept, LOCTEXT("Virtual_Gamepad_Accept_Key", "Virtual Gamepad Accept"), GamepadKey | Virtual, EKeys::NAME_VirtualCategory), FPlatformInput::GetGamepadAcceptKey());
            // AddVirtualKey(FKeyDetails(EKeys::Virtual_Gamepad_Back, LOCTEXT("Virtual_Gamepad_Back_Key", "Virtual Gamepad Back"), GamepadKey | Virtual, EKeys::NAME_VirtualCategory), FPlatformInput::GetGamepadBackKey());
        };
#undef ADD_KEY
    }
};

constinit auto KeyMapping::key_mapping = make_key_mapping(ConsoleType::None);

} // portal


