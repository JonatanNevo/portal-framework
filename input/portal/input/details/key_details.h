//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/flags.h"
#include "portal/core/debug/assert.h"
#include "portal/core/strings/string_registry.h"

#include "portal/input/keys.h"

namespace portal::input
{
enum class PairedAxis: uint8_t
{
    Unpaired,
    X,
    Y,
    z
};

enum class KeyFlagsBits : uint16_t
{
    GamepadKey  = 1 << 0,
    MouseButton = 1 << 1,
    ModifierKey = 1 << 2,

    UpdateAxisWithoutSamples = 1 << 3,
    NotActionBindableKey     = 1 << 4,

    // All axis representations
    ButtonAxis = 1 << 5, // Analog 1D axis emulating a digital button press. E.g. Gamepad right stick up
    Axis1D     = 1 << 6,
    Axis2D     = 1 << 7,
    Axis3D     = 1 << 8,

    // A key is "Virtual" if it is an abstract key whose actual value may change dependent on the platform.
    // For example, the standard "Accept" button on some platforms may be Gamepad_FaceButton_Down, while on
    // other platforms it may be Gamepad_FaceButton_Right.
    Virtual = 1 << 9,

    NoFlags = 0,
};

using KeyFlags = Flags<KeyFlagsBits>;


class KeyDetails
{
    enum class InputAxisType
    {
        None,
        Button,
        Axis1D,
        Axis2D,
        Axis3D
    };

public:
    constexpr KeyDetails(
        const Key key,
        const std::string_view long_display_name,
        const std::string_view short_display_name = "",
        const KeyFlags flags = KeyFlagsBits::NoFlags,
        const std::string_view menu_category = INVALID_STRING_VIEW
    ) : key(key), menu_category(menu_category), long_display_name(long_display_name), short_display_name(short_display_name)
    {
        common_init(flags);
    }


    [[nodiscard]] constexpr bool is_modifier_key() const
    {
        return flags & KeyFlagsBits::ModifierKey;
    }

    [[nodiscard]] constexpr bool is_gamepad_key() const
    {
        return flags & KeyFlagsBits::GamepadKey;
    }

    [[nodiscard]] constexpr bool is_mouse_button() const
    {
        return flags & KeyFlagsBits::MouseButton;
    }

    [[nodiscard]] constexpr bool is_axis_1d() const
    {
        return axis_type == InputAxisType::Axis1D;
    }

    [[nodiscard]] constexpr bool is_axis_2d() const
    {
        return axis_type == InputAxisType::Axis2D;
    }

    [[nodiscard]] constexpr bool is_axis_3d() const
    {
        return axis_type == InputAxisType::Axis3D;
    }

    [[nodiscard]] constexpr bool is_button_axis() const
    {
        return axis_type == InputAxisType::Button;
    }

    [[nodiscard]] constexpr bool is_analog() const
    {
        return is_axis_1d() || is_axis_2d() || is_axis_3d();
    }

    [[nodiscard]] constexpr bool is_digital() const
    {
        return !is_analog();
    }

    [[nodiscard]] constexpr bool should_update_axis_without_samples() const
    {
        return flags & KeyFlagsBits::UpdateAxisWithoutSamples;
    }

    [[nodiscard]] constexpr bool is_bindable_to_actions() const
    {
        return (flags & KeyFlagsBits::NotActionBindableKey) == 0;
    }

    [[nodiscard]] constexpr bool is_virtual() const
    {
        return flags & KeyFlagsBits::Virtual;
    }

    [[nodiscard]] std::string_view get_menu_category() const
    {
        return menu_category;
    }

    [[nodiscard]] std::string_view get_display_name(bool get_long = true) const
    {
        if (get_long || short_display_name.empty())
            return long_display_name;
        return short_display_name;
    }

    [[nodiscard]] const Key& get_key() const
    {
        return key;
    }

    [[nodiscard]] PairedAxis get_paired_axis() const
    {
        return paired_axis;
    }

    [[nodiscard]] const Key& get_paired_key() const
    {
        return paired_axis_key;
    }

    [[nodiscard]] const Key& get_virtual_key() const
    {
        return virtual_key_value;
    }

private:
    constexpr void common_init(const KeyFlags flags)
    {
        if (flags & KeyFlagsBits::ButtonAxis)
        {
            PORTAL_ASSERT((flags & (KeyFlagsBits::Axis1D | KeyFlagsBits::Axis2D | KeyFlagsBits::Axis3D))== 0, "Multiple axis flags set");
            axis_type = InputAxisType::Button;
        }
        else if (flags & KeyFlagsBits::Axis1D)
        {
            PORTAL_ASSERT((flags & (KeyFlagsBits::Axis2D | KeyFlagsBits::Axis3D))== 0, "Multiple axis flags set");
            axis_type = InputAxisType::Axis1D;
        }
        else if (flags & KeyFlagsBits::Axis2D)
        {
            PORTAL_ASSERT((flags & KeyFlagsBits::Axis3D)== 0, "Multiple axis flags set");
            axis_type = InputAxisType::Axis2D;
        }
        else if (flags & KeyFlagsBits::Axis3D)
        {
            axis_type = InputAxisType::Axis3D;
        }

        if (menu_category == INVALID_STRING_VIEW)
        {
            if (is_virtual())
                menu_category = "Virtual";
            else if (is_gamepad_key())
                menu_category = "Gamepad";
            else if (is_mouse_button())
                menu_category = "Mouse Buttons";
            else
                menu_category = "Keyboard";
        }
    }

    Key key;
    // Paired axis identifier. Lets this key know which axis it represents on the PairedAxisKey
    PairedAxis paired_axis = PairedAxis::Unpaired;
    // Paired axis reference. This is the Key representing the final paired vector axis. Note: NOT the other key in the pairing.
    Key paired_axis_key;

    /**
     * The actual FKey which should be used at runtime for virtual FKeys
     */
    Key virtual_key_value;

    std::string_view menu_category;
    KeyFlags flags;
    InputAxisType axis_type = InputAxisType::None;

    std::string_view long_display_name;
    std::string_view short_display_name;
};
}


template <>
struct portal::FlagTraits<portal::input::KeyFlagsBits>
{
    using enum portal::input::KeyFlagsBits;

    static constexpr bool is_bitmask = true;
    static constexpr auto all_flags = GamepadKey | MouseButton | ModifierKey | UpdateAxisWithoutSamples | NotActionBindableKey | ButtonAxis | Axis1D |
        Axis2D | Axis3D | Virtual;
};
