//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "input_types.h"

#include <ranges>

#include "portal/core/assert.h"
#include "portal/platform/input/hal/platform_input.h"

namespace portal
{

const Key Keys::AnyKey("AnyKey");

const Key Keys::MouseX("MouseX");
const Key Keys::MouseY("MouseY");
const Key Keys::Mouse2D("Mouse2D");
const Key Keys::MouseScrollUp("MouseScrollUp");
const Key Keys::MouseScrollDown("MouseScrollDown");
const Key Keys::MouseWheelAxis("MouseWheelAxis");

const Key Keys::LeftMouseButton("LeftMouseButton");
const Key Keys::RightMouseButton("RightMouseButton");
const Key Keys::MiddleMouseButton("MiddleMouseButton");
const Key Keys::ThumbMouseButton("ThumbMouseButton");
const Key Keys::ThumbMouseButton2("ThumbMouseButton2");

const Key Keys::BackSpace("BackSpace");
const Key Keys::Tab("Tab");
const Key Keys::Enter("Enter");
const Key Keys::Pause("Pause");

const Key Keys::CapsLock("CapsLock");
const Key Keys::Escape("Escape");
const Key Keys::SpaceBar("SpaceBar");
const Key Keys::PageUp("PageUp");
const Key Keys::PageDown("PageDown");
const Key Keys::End("End");
const Key Keys::Home("Home");

const Key Keys::Left("Left");
const Key Keys::Up("Up");
const Key Keys::Right("Right");
const Key Keys::Down("Down");

const Key Keys::Insert("Insert");
const Key Keys::Delete("Delete");

const Key Keys::Zero("Zero");
const Key Keys::One("One");
const Key Keys::Two("Two");
const Key Keys::Three("Three");
const Key Keys::Four("Four");
const Key Keys::Five("Five");
const Key Keys::Six("Six");
const Key Keys::Seven("Seven");
const Key Keys::Eight("Eight");
const Key Keys::Nine("Nine");

const Key Keys::A("A");
const Key Keys::B("B");
const Key Keys::C("C");
const Key Keys::D("D");
const Key Keys::E("E");
const Key Keys::F("F");
const Key Keys::G("G");
const Key Keys::H("H");
const Key Keys::I("I");
const Key Keys::J("J");
const Key Keys::K("K");
const Key Keys::L("L");
const Key Keys::M("M");
const Key Keys::N("N");
const Key Keys::O("O");
const Key Keys::P("P");
const Key Keys::Q("Q");
const Key Keys::R("R");
const Key Keys::S("S");
const Key Keys::T("T");
const Key Keys::U("U");
const Key Keys::V("V");
const Key Keys::W("W");
const Key Keys::X("X");
const Key Keys::Y("Y");
const Key Keys::Z("Z");

const Key Keys::NumPadZero("NumPadZero");
const Key Keys::NumPadOne("NumPadOne");
const Key Keys::NumPadTwo("NumPadTwo");
const Key Keys::NumPadThree("NumPadThree");
const Key Keys::NumPadFour("NumPadFour");
const Key Keys::NumPadFive("NumPadFive");
const Key Keys::NumPadSix("NumPadSix");
const Key Keys::NumPadSeven("NumPadSeven");
const Key Keys::NumPadEight("NumPadEight");
const Key Keys::NumPadNine("NumPadNine");

const Key Keys::Multiply("Multiply");
const Key Keys::Add("Add");
const Key Keys::Subtract("Subtract");
const Key Keys::Decimal("Decimal");
const Key Keys::Divide("Divide");

const Key Keys::F1("F1");
const Key Keys::F2("F2");
const Key Keys::F3("F3");
const Key Keys::F4("F4");
const Key Keys::F5("F5");
const Key Keys::F6("F6");
const Key Keys::F7("F7");
const Key Keys::F8("F8");
const Key Keys::F9("F9");
const Key Keys::F10("F10");
const Key Keys::F11("F11");
const Key Keys::F12("F12");

const Key Keys::NumLock("NumLock");

const Key Keys::ScrollLock("ScrollLock");

const Key Keys::LeftShift("LeftShift");
const Key Keys::RightShift("RightShift");
const Key Keys::LeftControl("LeftControl");
const Key Keys::RightControl("RightControl");
const Key Keys::LeftAlt("LeftAlt");
const Key Keys::RightAlt("RightAlt");
const Key Keys::LeftCommand("LeftCommand");
const Key Keys::RightCommand("RightCommand");

const Key Keys::Semicolon("Semicolon");
const Key Keys::Equals("Equals");
const Key Keys::Comma("Comma");
const Key Keys::Underscore("Underscore");
const Key Keys::Hyphen("Hyphen");
const Key Keys::Period("Period");
const Key Keys::Slash("Slash");
const Key Keys::Tilde("Tilde");
const Key Keys::LeftBracket("LeftBracket");
const Key Keys::LeftParantheses("LeftParantheses");
const Key Keys::Backslash("Backslash");
const Key Keys::RightBracket("RightBracket");
const Key Keys::RightParantheses("RightParantheses");
const Key Keys::Apostrophe("Apostrophe");
const Key Keys::Quote("Quote");

const Key Keys::Asterix("Asterix");
const Key Keys::Ampersand("Ampersand");
const Key Keys::Caret("Caret");
const Key Keys::Dollar("Dollar");
const Key Keys::Exclamation("Exclamation");
const Key Keys::Colon("Colon");

const Key Keys::A_AccentGrave("A_AccentGrave");
const Key Keys::E_AccentGrave("E_AccentGrave");
const Key Keys::E_AccentAigu("E_AccentAigu");
const Key Keys::C_Cedille("C_Cedille");
const Key Keys::Section("Section");

// Setup platform specific keys
// TODO: Change based on platform
const Key Keys::Platform_Delete = Keys::Delete;

// Ensure that the Gamepad_ names match those in GenericApplication.cpp
const Key Keys::Gamepad_Left2D("Gamepad_Left2D");
const Key Keys::Gamepad_LeftX("Gamepad_LeftX");
const Key Keys::Gamepad_LeftY("Gamepad_LeftY");
const Key Keys::Gamepad_Right2D("Gamepad_Right2D");
const Key Keys::Gamepad_RightX("Gamepad_RightX");
const Key Keys::Gamepad_RightY("Gamepad_RightY");
const Key Keys::Gamepad_LeftTriggerAxis("Gamepad_LeftTriggerAxis");
const Key Keys::Gamepad_RightTriggerAxis("Gamepad_RightTriggerAxis");

const Key Keys::Gamepad_LeftThumbstick("Gamepad_LeftThumbstick");
const Key Keys::Gamepad_RightThumbstick("Gamepad_RightThumbstick");
const Key Keys::Gamepad_Special_Left("Gamepad_Special_Left");
const Key Keys::Gamepad_Special_Left_X("Gamepad_Special_Left_X");
const Key Keys::Gamepad_Special_Left_Y("Gamepad_Special_Left_Y");
const Key Keys::Gamepad_Special_Right("Gamepad_Special_Right");
const Key Keys::Gamepad_FaceButton_Bottom("Gamepad_FaceButton_Bottom");
const Key Keys::Gamepad_FaceButton_Right("Gamepad_FaceButton_Right");
const Key Keys::Gamepad_FaceButton_Left("Gamepad_FaceButton_Left");
const Key Keys::Gamepad_FaceButton_Top("Gamepad_FaceButton_Top");
const Key Keys::Gamepad_LeftShoulder("Gamepad_LeftShoulder");
const Key Keys::Gamepad_RightShoulder("Gamepad_RightShoulder");
const Key Keys::Gamepad_LeftTrigger("Gamepad_LeftTrigger");
const Key Keys::Gamepad_RightTrigger("Gamepad_RightTrigger");
const Key Keys::Gamepad_DPad_Up("Gamepad_DPad_Up");
const Key Keys::Gamepad_DPad_Down("Gamepad_DPad_Down");
const Key Keys::Gamepad_DPad_Right("Gamepad_DPad_Right");
const Key Keys::Gamepad_DPad_Left("Gamepad_DPad_Left");

// Virtual key codes used for input axis button press/release emulation
const Key Keys::Gamepad_LeftStick_Up("Gamepad_LeftStick_Up");
const Key Keys::Gamepad_LeftStick_Down("Gamepad_LeftStick_Down");
const Key Keys::Gamepad_LeftStick_Right("Gamepad_LeftStick_Right");
const Key Keys::Gamepad_LeftStick_Left("Gamepad_LeftStick_Left");

const Key Keys::Gamepad_RightStick_Up("Gamepad_RightStick_Up");
const Key Keys::Gamepad_RightStick_Down("Gamepad_RightStick_Down");
const Key Keys::Gamepad_RightStick_Right("Gamepad_RightStick_Right");
const Key Keys::Gamepad_RightStick_Left("Gamepad_RightStick_Left");

// Fingers
const Key Keys::TouchKeys[NUM_TOUCH_KEYS];

// Gestures
const Key Keys::Gesture_Pinch("Gesture_Pinch");
const Key Keys::Gesture_Flick("Gesture_Flick");
const Key Keys::Gesture_Rotate("Gesture_Rotate");

// Steam Controller Specific
const Key Keys::Steam_Touch_0("Steam_Touch_0");
const Key Keys::Steam_Touch_1("Steam_Touch_1");
const Key Keys::Steam_Touch_2("Steam_Touch_2");
const Key Keys::Steam_Touch_3("Steam_Touch_3");
const Key Keys::Steam_Back_Left("Steam_Back_Left");
const Key Keys::Steam_Back_Right("Steam_Back_Right");

// Android-specific
const Key Keys::Android_Back("Android_Back");
const Key Keys::Android_Volume_Up("Android_Volume_Up");
const Key Keys::Android_Volume_Down("Android_Volume_Down");
const Key Keys::Android_Menu("Android_Menu");

const Key Keys::Invalid("Invalid");

// TODO: Changed based on controller
const Key Keys::Virtual_Accept = Keys::Gamepad_FaceButton_Bottom;
const Key Keys::Virtual_Back = Keys::Gamepad_FaceButton_Right;

bool Keys::initialized = false;
std::unordered_map<Key, std::shared_ptr<KeyDetails>> Keys::input_keys;

bool Key::is_valid() const
{
    if (name != "")
    {
        conditional_lookup_key_details();
        return details != nullptr;

    }
    return false;
}

bool Key::is_modifier_key() const
{
    conditional_lookup_key_details();
    return (details && details->is_modifier_key());
}

bool Key::is_gamepad_key() const
{
    return details && details->is_gamepad_key();
}

bool Key::is_touch() const
{
    conditional_lookup_key_details();
    return (details && details->is_touch());
}

bool Key::is_mouse_button() const
{
    conditional_lookup_key_details();
    return (details && details->is_mouse_button());
}

bool Key::is_button_axis() const
{
    conditional_lookup_key_details();
    return (details && details->is_button_axis());
}

bool Key::is_axis_1D() const
{
    conditional_lookup_key_details();
    return (details && details->is_axis_1D());
}

bool Key::is_axis_2D() const
{
    conditional_lookup_key_details();
    return (details && details->is_axis_2D());
}

bool Key::is_digital() const
{
    conditional_lookup_key_details();
    return (details && details->is_digital());
}

bool Key::is_analog() const
{
    conditional_lookup_key_details();
    return (details && details->is_analog());
}

bool Key::should_update_axis_without_samples() const
{
    conditional_lookup_key_details();
    return (details && details->should_update_axis_without_samples());
}

bool Key::is_gesture() const
{
    conditional_lookup_key_details();
    return (details && details->is_gesture());
}

std::string Key::get_name() const
{
    return std::string(name);
}

PairedAxis Key::get_paired_axis() const
{
    conditional_lookup_key_details();
    if (details)
    {
        return details->get_paired_axis();
    }
    return PairedAxis::Unpaired;
}

Key Key::get_paired_axis_key() const
{
    conditional_lookup_key_details();
    if (details)
    {
        return details->get_paired_axis_key();
    }
    return Key();
}

void Key::reset_key() const
{
    details.reset();
}

void Key::conditional_lookup_key_details() const
{
    if (details == nullptr)
    {
        details = Keys::get_key_details(*this);
    }
}

KeyDetails::KeyDetails(const Key& key, const uint32_t flags):
    key(key)
{
    b_is_modifier_key = ((flags & KeyFlag::ModifierKey) != 0);
    b_is_gamepad_key = ((flags & KeyFlag::GamepadKey) != 0);
    b_is_touch = ((flags & KeyFlag::Touch) != 0);
    b_is_mouse_button = ((flags & KeyFlag::MouseButton) != 0);
    b_should_update_axis_without_samples = ((flags & KeyFlag::UpdateAxisWithoutSamples) != 0);

    if ((flags & KeyFlag::ButtonAxis) != 0)
    {
        PORTAL_ASSERT((flags & (KeyFlag::Axis1D | KeyFlag::Axis2D)) == 0, "Key cannot be bound to multiple types of axis");
        axis_type = InputAxisType::Button;
    }
    else if ((flags & KeyFlag::Axis1D) != 0)
    {
        PORTAL_ASSERT((flags & (KeyFlag::Axis2D | KeyFlag::ButtonAxis)) == 0, "Key cannot be bound to multiple types of axis");
        axis_type = InputAxisType::Axis1D;
    }
    else if ((flags & KeyFlag::Axis2D) != 0)
    {
        PORTAL_ASSERT((flags & (KeyFlag::Axis1D | KeyFlag::ButtonAxis)) == 0, "Key cannot be bound to multiple types of axis");
        axis_type = InputAxisType::Axis2D;
    }
    else
    {
        axis_type = InputAxisType::None;
    }
}

bool KeyDetails::is_modifier_key() const
{
    return b_is_modifier_key;
}

bool KeyDetails::is_gamepad_key() const
{
    return b_is_gamepad_key;
}

bool KeyDetails::is_touch() const
{
    return b_is_touch;
}

bool KeyDetails::is_mouse_button() const
{
    return b_is_mouse_button;
}

bool KeyDetails::is_axis_1D() const
{
    return axis_type == InputAxisType::Axis1D;
}

bool KeyDetails::is_axis_2D() const
{
    return axis_type == InputAxisType::Axis2D;
}

bool KeyDetails::is_button_axis() const
{
    return axis_type == InputAxisType::Button;
}

bool KeyDetails::is_analog() const
{
    return is_axis_1D() || is_axis_2D();
}

bool KeyDetails::is_digital() const
{
    return !is_analog();
}

bool KeyDetails::should_update_axis_without_samples() const
{
    return b_should_update_axis_without_samples;
}

bool KeyDetails::is_gesture() const
{
    return b_is_gesture;
}

const Key& KeyDetails::get_key() const
{
    return key;
}

PairedAxis KeyDetails::get_paired_axis() const
{
    return paired_axis;
}

Key KeyDetails::get_paired_axis_key() const
{
    return paired_axis_key;
}

void Keys::initialize()
{
    if (initialized)
        return;
    initialized = true;

    for (auto touch_input = 0; touch_input < NUM_TOUCH_KEYS; ++touch_input)
    {
        const_cast<Key&>(Keys::TouchKeys[touch_input]) = Key(std::format("Touch{}", touch_input + 1));
    }

    add_key(KeyDetails(Keys::AnyKey));;
    add_key(KeyDetails(Keys::MouseX, KeyDetails::Axis1D | KeyDetails::MouseButton | KeyDetails::UpdateAxisWithoutSamples));
    add_key(KeyDetails(Keys::MouseY, KeyDetails::Axis1D | KeyDetails::MouseButton | KeyDetails::UpdateAxisWithoutSamples));
    add_paired_key(
        KeyDetails(Keys::Mouse2D, KeyDetails::Axis2D | KeyDetails::MouseButton | KeyDetails::UpdateAxisWithoutSamples),
        Keys::MouseX,
        Keys::MouseY);
    add_key(KeyDetails(Keys::MouseWheelAxis, KeyDetails::Axis1D | KeyDetails::MouseButton | KeyDetails::UpdateAxisWithoutSamples));
    add_key(KeyDetails(Keys::MouseScrollUp, KeyDetails::MouseButton | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::MouseScrollDown, KeyDetails::MouseButton | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::LeftMouseButton, KeyDetails::MouseButton));
    add_key(KeyDetails(Keys::RightMouseButton, KeyDetails::MouseButton));
    add_key(KeyDetails(Keys::MiddleMouseButton, KeyDetails::MouseButton));
    add_key(KeyDetails(Keys::ThumbMouseButton, KeyDetails::MouseButton));
    add_key(KeyDetails(Keys::ThumbMouseButton2, KeyDetails::MouseButton));
    add_key(KeyDetails(Keys::Tab));
    add_key(KeyDetails(Keys::Enter));
    add_key(KeyDetails(Keys::Pause));
    add_key(KeyDetails(Keys::CapsLock));
    add_key(KeyDetails(Keys::Escape));
    add_key(KeyDetails(Keys::SpaceBar));
    add_key(KeyDetails(Keys::PageUp));
    add_key(KeyDetails(Keys::PageDown));
    add_key(KeyDetails(Keys::End));
    add_key(KeyDetails(Keys::Home));
    add_key(KeyDetails(Keys::Left));
    add_key(KeyDetails(Keys::Up));
    add_key(KeyDetails(Keys::Right));
    add_key(KeyDetails(Keys::Down));
    add_key(KeyDetails(Keys::Insert));

#if PORTAL_PLATFORM_MAC
    add_key(KeyDetails(Keys::BackSpace));
    add_key(KeyDetails(Keys::Delete));
#else
    add_key(KeyDetails(Keys::BackSpace));
    add_key(KeyDetails(Keys::Delete));
#endif

    add_key(KeyDetails(Keys::Zero));
    add_key(KeyDetails(Keys::One));
    add_key(KeyDetails(Keys::Two));
    add_key(KeyDetails(Keys::Three));
    add_key(KeyDetails(Keys::Four));
    add_key(KeyDetails(Keys::Five));
    add_key(KeyDetails(Keys::Six));
    add_key(KeyDetails(Keys::Seven));
    add_key(KeyDetails(Keys::Eight));
    add_key(KeyDetails(Keys::Nine));
    add_key(KeyDetails(Keys::A));
    add_key(KeyDetails(Keys::B));
    add_key(KeyDetails(Keys::C));
    add_key(KeyDetails(Keys::D));
    add_key(KeyDetails(Keys::E));
    add_key(KeyDetails(Keys::F));
    add_key(KeyDetails(Keys::G));
    add_key(KeyDetails(Keys::H));
    add_key(KeyDetails(Keys::I));
    add_key(KeyDetails(Keys::J));
    add_key(KeyDetails(Keys::K));
    add_key(KeyDetails(Keys::L));
    add_key(KeyDetails(Keys::M));
    add_key(KeyDetails(Keys::N));
    add_key(KeyDetails(Keys::O));
    add_key(KeyDetails(Keys::P));
    add_key(KeyDetails(Keys::Q));
    add_key(KeyDetails(Keys::R));
    add_key(KeyDetails(Keys::S));
    add_key(KeyDetails(Keys::T));
    add_key(KeyDetails(Keys::U));
    add_key(KeyDetails(Keys::V));
    add_key(KeyDetails(Keys::W));
    add_key(KeyDetails(Keys::X));
    add_key(KeyDetails(Keys::Y));
    add_key(KeyDetails(Keys::Z));

    add_key(KeyDetails(Keys::NumPadZero));
    add_key(KeyDetails(Keys::NumPadOne));
    add_key(KeyDetails(Keys::NumPadTwo));
    add_key(KeyDetails(Keys::NumPadThree));
    add_key(KeyDetails(Keys::NumPadFour));
    add_key(KeyDetails(Keys::NumPadFive));
    add_key(KeyDetails(Keys::NumPadSix));
    add_key(KeyDetails(Keys::NumPadSeven));
    add_key(KeyDetails(Keys::NumPadEight));
    add_key(KeyDetails(Keys::NumPadNine));

    add_key(KeyDetails(Keys::Multiply));
    add_key(KeyDetails(Keys::Add));
    add_key(KeyDetails(Keys::Subtract));
    add_key(KeyDetails(Keys::Decimal));
    add_key(KeyDetails(Keys::Divide));

    add_key(KeyDetails(Keys::F1));
    add_key(KeyDetails(Keys::F2));
    add_key(KeyDetails(Keys::F3));
    add_key(KeyDetails(Keys::F4));
    add_key(KeyDetails(Keys::F5));
    add_key(KeyDetails(Keys::F6));
    add_key(KeyDetails(Keys::F7));
    add_key(KeyDetails(Keys::F8));
    add_key(KeyDetails(Keys::F9));
    add_key(KeyDetails(Keys::F10));
    add_key(KeyDetails(Keys::F11));
    add_key(KeyDetails(Keys::F12));

    add_key(KeyDetails(Keys::NumLock));
    add_key(KeyDetails(Keys::ScrollLock));

    add_key(KeyDetails(Keys::LeftShift, KeyDetails::ModifierKey));
    add_key(KeyDetails(Keys::RightShift, KeyDetails::ModifierKey));
    add_key(KeyDetails(Keys::LeftControl, KeyDetails::ModifierKey));
    add_key(KeyDetails(Keys::RightControl, KeyDetails::ModifierKey));
    add_key(KeyDetails(Keys::LeftAlt, KeyDetails::ModifierKey));
    add_key(KeyDetails(Keys::RightAlt, KeyDetails::ModifierKey));
    add_key(KeyDetails(Keys::LeftCommand, KeyDetails::ModifierKey));
    add_key(KeyDetails(Keys::RightCommand, KeyDetails::ModifierKey));

    add_key(KeyDetails(Keys::Semicolon));
    add_key(KeyDetails(Keys::Equals));
    add_key(KeyDetails(Keys::Comma));
    add_key(KeyDetails(Keys::Hyphen));
    add_key(KeyDetails(Keys::Underscore));
    add_key(KeyDetails(Keys::Period));
    add_key(KeyDetails(Keys::Slash));
    add_key(KeyDetails(Keys::Tilde));
    add_key(KeyDetails(Keys::LeftBracket));
    add_key(KeyDetails(Keys::Backslash));
    add_key(KeyDetails(Keys::RightBracket));
    add_key(KeyDetails(Keys::Apostrophe));
    add_key(KeyDetails(Keys::Quote));

    add_key(KeyDetails(Keys::LeftParantheses));
    add_key(KeyDetails(Keys::RightParantheses));
    add_key(KeyDetails(Keys::Ampersand));
    add_key(KeyDetails(Keys::Asterix));
    add_key(KeyDetails(Keys::Caret));
    add_key(KeyDetails(Keys::Dollar));
    add_key(KeyDetails(Keys::Exclamation));
    add_key(KeyDetails(Keys::Colon));

    add_key(KeyDetails(Keys::A_AccentGrave));
    add_key(KeyDetails(Keys::E_AccentGrave));
    add_key(KeyDetails(Keys::E_AccentAigu));
    add_key(KeyDetails(Keys::C_Cedille));
    add_key(KeyDetails(Keys::Section));


    // Setup Gamepad keys
    add_key(KeyDetails(Keys::Gamepad_LeftX, KeyDetails::GamepadKey | KeyDetails::Axis1D));
    add_key(KeyDetails(Keys::Gamepad_LeftY, KeyDetails::GamepadKey | KeyDetails::Axis1D));
    add_paired_key(KeyDetails(Keys::Gamepad_Left2D, KeyDetails::GamepadKey | KeyDetails::Axis2D), Keys::Gamepad_LeftX, Keys::Gamepad_LeftY);
    add_key(KeyDetails(Keys::Gamepad_RightX, KeyDetails::GamepadKey | KeyDetails::Axis1D));
    add_key(KeyDetails(Keys::Gamepad_RightY, KeyDetails::GamepadKey | KeyDetails::Axis1D));
    add_paired_key(KeyDetails(Keys::Gamepad_Right2D, KeyDetails::GamepadKey | KeyDetails::Axis2D), Keys::Gamepad_RightX, Keys::Gamepad_RightY);

    add_key(KeyDetails(Keys::Gamepad_DPad_Up, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_DPad_Down, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_DPad_Right, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_DPad_Left, KeyDetails::GamepadKey));

    // Virtual key codes used for input axis button press/release emulation
    add_key(KeyDetails(Keys::Gamepad_LeftStick_Up, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::Gamepad_LeftStick_Down, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::Gamepad_LeftStick_Right, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::Gamepad_LeftStick_Left, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));

    add_key(KeyDetails(Keys::Gamepad_RightStick_Up, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::Gamepad_RightStick_Down, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::Gamepad_RightStick_Right, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::Gamepad_RightStick_Left, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));

    add_key(KeyDetails(Keys::Gamepad_Special_Left, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_Special_Right, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_FaceButton_Bottom, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_FaceButton_Right, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_FaceButton_Left, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_FaceButton_Top, KeyDetails::GamepadKey));

    add_key(KeyDetails(Keys::Gamepad_LeftTriggerAxis, KeyDetails::GamepadKey | KeyDetails::Axis1D));
    add_key(KeyDetails(Keys::Gamepad_RightTriggerAxis, KeyDetails::GamepadKey | KeyDetails::Axis1D));

    add_key(KeyDetails(Keys::Gamepad_LeftShoulder, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_RightShoulder, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_LeftTrigger, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));
    add_key(KeyDetails(Keys::Gamepad_RightTrigger, KeyDetails::GamepadKey | KeyDetails::ButtonAxis));

    add_key(KeyDetails(Keys::Gamepad_LeftThumbstick, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Gamepad_RightThumbstick, KeyDetails::GamepadKey));

    static_assert(
        Keys::NUM_TOUCH_KEYS == static_cast<int>(TouchIndex::MAX_TOUCHES),
        "The number of touch keys should be equal to the max number of TouchIndexes");
    for (int touch_index = 0; touch_index < (Keys::NUM_TOUCH_KEYS - 1); touch_index++)
    {
        add_key(KeyDetails(Keys::TouchKeys[touch_index], KeyDetails::Touch));
    }

    add_key(KeyDetails(Keys::Gamepad_Special_Left_X, KeyDetails::GamepadKey | KeyDetails::Axis1D));
    add_key(KeyDetails(Keys::Gamepad_Special_Left_Y, KeyDetails::GamepadKey | KeyDetails::Axis1D));

    add_key(KeyDetails(Keys::Steam_Touch_0, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Steam_Touch_1, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Steam_Touch_2, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Steam_Touch_3, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Steam_Back_Left, KeyDetails::GamepadKey));
    add_key(KeyDetails(Keys::Steam_Back_Right, KeyDetails::GamepadKey));

    // Initialize the input key manager.  This will cause any additional OEM keys to get added
    InputKeyManager::get();
}

void Keys::add_key(const KeyDetails& key_details)
{
    const Key& key = key_details.key;
    PORTAL_ASSERT(!input_keys.contains(key), "Adding duplicate key");
    key.details = std::make_shared<KeyDetails>(key_details);
    input_keys[key] = key.details;
}

void Keys::add_paired_key(const KeyDetails& paired_key_details, Key x, Key y)
{
    add_key(paired_key_details);
    input_keys[x]->paired_axis = PairedAxis::X;
    input_keys[y]->paired_axis = PairedAxis::Y;
    input_keys[x]->paired_axis_key = input_keys[y]->paired_axis_key = paired_key_details.key;
}

void Keys::get_all_key(std::vector<Key>& out_keys)
{
    out_keys.reserve(input_keys.size());
    for (const auto& key : input_keys | std::views::keys)
    {
        out_keys.push_back(key);
    }
}

std::shared_ptr<KeyDetails> Keys::get_key_details(const Key& key)
{
    const auto details = input_keys.find(key);
    if (details == input_keys.end())
    {
        return nullptr;
    }
    return details->second;
}

std::shared_ptr<InputKeyManager> InputKeyManager::instance;

InputKeyManager& InputKeyManager::get()
{
    if (instance == nullptr)
    {
        instance = std::shared_ptr<InputKeyManager>(new InputKeyManager());
    }
    return *instance;
}

void InputKeyManager::get_codes_from_key(const Key& key, uint32_t& key_code) const
{
    const auto key_it = std::ranges::find_if(keymap_virtual_to_enum, [&key](const auto& pair) { return pair.second == key; });
    if (key_it != keymap_virtual_to_enum.end())
    {
        key_code = key_it->first;
    }
}

constexpr auto unknown_char_code = "UnknownCharCode_"sv;

Key InputKeyManager::get_key_from_codes(const uint32_t key_code) const
{
    const auto key_it = keymap_virtual_to_enum.find(key_code);
    return key_it != keymap_virtual_to_enum.end() ? key_it->second : Keys::Invalid;
}

void InputKeyManager::init_key_mapping()
{
    static constexpr uint32_t MAX_KEY_MAPPINGS = 256;
    std::vector<uint32_t> key_codes(MAX_KEY_MAPPINGS);
    std::vector<std::string> key_names(MAX_KEY_MAPPINGS);
    const uint32_t keymap_size = input::PlatformInput::get_keymap(key_codes, key_names, MAX_KEY_MAPPINGS);

    // When the input language changes, a key that was virtual may no longer be virtual.
    // We must repopulate the maps to ensure GetKeyFromCodes returns the correct value per language.
    keymap_virtual_to_enum.clear();

    for (size_t i = 0; i < keymap_size; i++)
    {
        Key key(key_names[i]);
        if (!key.is_valid())
        {
            Keys::add_key(KeyDetails(key));
        }

        keymap_virtual_to_enum[key_codes[i]] = key;
    }
}

}
