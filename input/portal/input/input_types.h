//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <cstdint>
#include <string_view>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace portal
{
using namespace std::string_view_literals;

enum class InputEvent: uint8_t
{
    Pressed = 0,
    Released = 1,
    Repeat = 2,
    DoubleClick = 3,
    Axis = 4,
    Max = 5,
};

enum class PairedAxis: uint8_t
{
    Unpaired, // This key is unpaired
    X,        // This key represents the X axis of its PairedAxisKey
    Y,        // This key represents the Y axis of its PairedAxisKey
    Z,        // This key represents the Z axis of its PairedAxisKey - Currently unused
};

struct Key
{
    Key() = default;

    explicit Key(const std::string& name) :
        name(name)
    {
    }

    bool is_valid() const;
    bool is_modifier_key() const;
    bool is_gamepad_key() const;
    bool is_touch() const;
    bool is_mouse_button() const;
    bool is_button_axis() const;
    bool is_axis_1D() const;
    bool is_axis_2D() const;
    bool is_digital() const;
    bool is_analog() const;
    bool should_update_axis_without_samples() const;
    bool is_gesture() const;

    std::string get_name() const;
    PairedAxis get_paired_axis() const;
    Key get_paired_axis_key() const;

    friend bool operator==(const Key& key_a, const Key& key_b) { return key_a.name == key_b.name; }
    friend bool operator!=(const Key& key_a, const Key& key_b) { return key_a.name != key_b.name; }
    friend struct Keys;

private:
    void reset_key() const;
    void conditional_lookup_key_details() const;

private:
    std::string name = "";

    mutable class std::shared_ptr<struct KeyDetails> details;
};

struct KeyDetails
{
    // Non enum class for ease of use in bit operations
    enum KeyFlag: uint8_t
    {
        GamepadKey = 1 << 0,
        Touch = 1 << 1,
        MouseButton = 1 << 2,
        ModifierKey = 1 << 3,
        Axis1D = 1 << 4,
        Axis2D = 1 << 5,
        UpdateAxisWithoutSamples = 1 << 6,
        ButtonAxis = 1 << 7, // Analog 1D axis emulating a digital button press. E.g. Gamepad right stick up
        NoFlags = 0
    };


    explicit KeyDetails(const Key& key, uint32_t flags = 0);
    bool is_modifier_key() const;
    bool is_gamepad_key() const;
    bool is_touch() const;
    bool is_mouse_button() const;
    bool is_axis_1D() const;
    bool is_axis_2D() const;
    bool is_button_axis() const;
    bool is_analog() const;
    bool is_digital() const;
    bool should_update_axis_without_samples() const;
    bool is_gesture() const;
    const Key& get_key() const;

    PairedAxis get_paired_axis() const;
    Key get_paired_axis_key() const;

private:
    friend struct Keys;

    enum class InputAxisType : uint8_t
    {
        None,
        Button, // Whilst the physical input is an analog axis the FKey uses it to emulate a digital button
        Axis1D,
        Axis2D,
        Axis3D,
    };

    Key key;

    // Paired axis identifier. Lets this key know which axis it represents on the PairedAxisKey
    PairedAxis paired_axis = PairedAxis::Unpaired;
    // Paired axis reference. This is the Key representing the final paired vector axis. Note: NOT the other key in the pairing.
    Key paired_axis_key;

    bool b_is_modifier_key : 1;
    bool b_is_gamepad_key : 1;
    bool b_is_touch : 1;
    bool b_is_mouse_button : 1;
    bool b_should_update_axis_without_samples : 1;
    bool b_is_gesture : 1;
    InputAxisType axis_type;
};

enum class TouchIndex : int
{
    Touch1,
    Touch2,
    Touch3,
    Touch4,
    Touch5,
    Touch6,
    Touch7,
    Touch8,
    Touch9,
    Touch10,
    /**
     * This entry is special.  NUM_TOUCH_KEYS - 1, is used for the cursor so that it's represented
     * as another finger index, but doesn't overlap with touch input indexes.
     */
    CursorPointerIndex,
    MAX_TOUCHES
};

enum class ConsoleGamepadLabel
{
    None,
    XBox,
    PS,
    Switch
};

struct Keys
{
    static const Key AnyKey;

    static const Key MouseX;
    static const Key MouseY;
    static const Key Mouse2D;
    static const Key MouseScrollUp;
    static const Key MouseScrollDown;
    static const Key MouseWheelAxis;

    static const Key LeftMouseButton;
    static const Key RightMouseButton;
    static const Key MiddleMouseButton;
    static const Key ThumbMouseButton;
    static const Key ThumbMouseButton2;

    static const Key BackSpace;
    static const Key Tab;
    static const Key Enter;
    static const Key Pause;

    static const Key CapsLock;
    static const Key Escape;
    static const Key SpaceBar;
    static const Key PageUp;
    static const Key PageDown;
    static const Key End;
    static const Key Home;

    static const Key Left;
    static const Key Up;
    static const Key Right;
    static const Key Down;

    static const Key Insert;
    static const Key Delete;

    static const Key Zero;
    static const Key One;
    static const Key Two;
    static const Key Three;
    static const Key Four;
    static const Key Five;
    static const Key Six;
    static const Key Seven;
    static const Key Eight;
    static const Key Nine;

    static const Key A;
    static const Key B;
    static const Key C;
    static const Key D;
    static const Key E;
    static const Key F;
    static const Key G;
    static const Key H;
    static const Key I;
    static const Key J;
    static const Key K;
    static const Key L;
    static const Key M;
    static const Key N;
    static const Key O;
    static const Key P;
    static const Key Q;
    static const Key R;
    static const Key S;
    static const Key T;
    static const Key U;
    static const Key V;
    static const Key W;
    static const Key X;
    static const Key Y;
    static const Key Z;

    static const Key NumPadZero;
    static const Key NumPadOne;
    static const Key NumPadTwo;
    static const Key NumPadThree;
    static const Key NumPadFour;
    static const Key NumPadFive;
    static const Key NumPadSix;
    static const Key NumPadSeven;
    static const Key NumPadEight;
    static const Key NumPadNine;

    static const Key Multiply;
    static const Key Add;
    static const Key Subtract;
    static const Key Decimal;
    static const Key Divide;

    static const Key F1;
    static const Key F2;
    static const Key F3;
    static const Key F4;
    static const Key F5;
    static const Key F6;
    static const Key F7;
    static const Key F8;
    static const Key F9;
    static const Key F10;
    static const Key F11;
    static const Key F12;

    static const Key NumLock;

    static const Key ScrollLock;

    static const Key LeftShift;
    static const Key RightShift;
    static const Key LeftControl;
    static const Key RightControl;
    static const Key LeftAlt;
    static const Key RightAlt;
    static const Key LeftCommand;
    static const Key RightCommand;

    static const Key Semicolon;
    static const Key Equals;
    static const Key Comma;
    static const Key Underscore;
    static const Key Hyphen;
    static const Key Period;
    static const Key Slash;
    static const Key Tilde;
    static const Key LeftBracket;
    static const Key Backslash;
    static const Key RightBracket;
    static const Key Apostrophe;

    static const Key Ampersand;
    static const Key Asterix;
    static const Key Caret;
    static const Key Colon;
    static const Key Dollar;
    static const Key Exclamation;
    static const Key LeftParantheses;
    static const Key RightParantheses;
    static const Key Quote;

    static const Key A_AccentGrave;
    static const Key E_AccentGrave;
    static const Key E_AccentAigu;
    static const Key C_Cedille;
    static const Key Section;

    // Platform Keys
    // These keys platform specific versions of keys that go by different names.
    // The delete key is a good example, on Windows Delete is the virtual key for Delete.
    // On Macs, the Delete key is the virtual key for BackSpace.
    static const Key Platform_Delete;

    // Gamepad Keys
    static const Key Gamepad_Left2D;
    static const Key Gamepad_LeftX;
    static const Key Gamepad_LeftY;
    static const Key Gamepad_Right2D;
    static const Key Gamepad_RightX;
    static const Key Gamepad_RightY;
    static const Key Gamepad_LeftTriggerAxis;
    static const Key Gamepad_RightTriggerAxis;

    static const Key Gamepad_LeftThumbstick;
    static const Key Gamepad_RightThumbstick;
    static const Key Gamepad_Special_Left;
    static const Key Gamepad_Special_Left_X;
    static const Key Gamepad_Special_Left_Y;
    static const Key Gamepad_Special_Right;
    static const Key Gamepad_FaceButton_Bottom;
    static const Key Gamepad_FaceButton_Right;
    static const Key Gamepad_FaceButton_Left;
    static const Key Gamepad_FaceButton_Top;
    static const Key Gamepad_LeftShoulder;
    static const Key Gamepad_RightShoulder;
    static const Key Gamepad_LeftTrigger;
    static const Key Gamepad_RightTrigger;
    static const Key Gamepad_DPad_Up;
    static const Key Gamepad_DPad_Down;
    static const Key Gamepad_DPad_Right;
    static const Key Gamepad_DPad_Left;

    // Virtual key codes used for input axis button press/release emulation
    static const Key Gamepad_LeftStick_Up;
    static const Key Gamepad_LeftStick_Down;
    static const Key Gamepad_LeftStick_Right;
    static const Key Gamepad_LeftStick_Left;

    static const Key Gamepad_RightStick_Up;
    static const Key Gamepad_RightStick_Down;
    static const Key Gamepad_RightStick_Right;
    static const Key Gamepad_RightStick_Left;

    // Gestures
    static const Key Gesture_Pinch;
    static const Key Gesture_Flick;
    static const Key Gesture_Rotate;

    // Steam Controller Specific
    static const Key Steam_Touch_0;
    static const Key Steam_Touch_1;
    static const Key Steam_Touch_2;
    static const Key Steam_Touch_3;
    static const Key Steam_Back_Left;
    static const Key Steam_Back_Right;

    // Android-specific
    static const Key Android_Back;
    static const Key Android_Volume_Up;
    static const Key Android_Volume_Down;
    static const Key Android_Menu;

    // Virtual buttons that use other buttons depending on the platform
    static const Key Virtual_Accept;
    static const Key Virtual_Back;

    static const Key Invalid;

    static const int32_t NUM_TOUCH_KEYS = 11;
    static const Key TouchKeys[NUM_TOUCH_KEYS];

    static ConsoleGamepadLabel ConsoleGamepadLabels;

    static void initialize();
    static void add_key(const KeyDetails& key_details);
    static void add_paired_key(const KeyDetails& paired_key_details, Key x, Key y); // Map the two provided keys to the X and Z axes of the paired key
    static void get_all_key(std::vector<Key>& out_keys);
    static std::shared_ptr<KeyDetails> get_key_details(const Key& key);

private:
    static std::unordered_map<Key, std::shared_ptr<KeyDetails>> input_keys;
    static bool initialized;
};

// Handles the mapping between the key codes and the `Key` struct
struct InputKeyManager
{
    static InputKeyManager& get();
    void get_codes_from_key(const Key& key, uint32_t& key_code) const;
    [[nodiscard]] Key get_key_from_codes(uint32_t key_code) const;
    void init_key_mapping();

private:
    InputKeyManager()
    {
        init_key_mapping();
    }


    static std::shared_ptr<InputKeyManager> instance;
    std::unordered_map<uint32_t, Key> keymap_virtual_to_enum;
};
}

template <>
struct std::hash<portal::Key>
{
    size_t operator()(const portal::Key& key) const noexcept
    {
        return std::hash<std::string_view>()(key.get_name());
    }
};
