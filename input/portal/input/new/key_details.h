//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/flags.h"
#include "portal/core/strings/string_id.h"
#include "portal/input/new/input_types.h"

namespace portal::ng
{

class KeyDetails
{
public:
    constexpr KeyDetails(
        const Key& key,
        const StringId& long_description,
        const KeyFlags flags = KeyFlags(KeyFlagsBits::Empty),
        const StringId& short_description = StringId()
        )
        : key(key),
          short_description(short_description),
          long_description(long_description)
    {
        populate_flags(flags);
    }

    constexpr KeyDetails(
        const Key& key,
        const StringId& long_description,
        const KeyFlags flags,
        const PairedAxis paired_axis,
        const Key& paired_axis_key,
        const StringId& short_description = StringId()
        )
        : key(key),
          axis(paired_axis),
          paired_axis_key(paired_axis_key),
          short_description(short_description),
          long_description(long_description)
    {
        populate_flags(flags);
    }

    [[nodiscard]] constexpr bool is_modifier_key() const { return modifier_key; }
    [[nodiscard]] constexpr bool is_gamepad_key() const { return gamepad_key; }
    [[nodiscard]] constexpr bool is_touch() const { return touch; }
    [[nodiscard]] constexpr bool is_mouse_button() const { return mouse_button; }
    [[nodiscard]] constexpr bool is_button_axis() const { return axis_type == AxisType::Button; }
    [[nodiscard]] constexpr bool is_axis_1D() const { return axis_type == AxisType::Axis1D; }
    [[nodiscard]] constexpr bool is_axis_2D() const { return axis_type == AxisType::Axis2D; }
    [[nodiscard]] constexpr bool is_axis_3D() const { return axis_type == AxisType::Axis3D; }
    [[nodiscard]] constexpr bool is_analog() const { return is_axis_1D() || is_axis_2D() || is_axis_3D(); }
    [[nodiscard]] constexpr bool is_digital() const { return !is_analog(); }
    [[nodiscard]] constexpr bool should_update_axis_without_samples() const { return update_axis_without_samples; }
    [[nodiscard]] constexpr bool is_virtual() const { return virtual_key; }

    [[nodiscard]] constexpr const Key& get_key() const { return key; }

    [[nodiscard]] constexpr PairedAxis get_paired_axis() const { return axis; }
    [[nodiscard]] constexpr const Key& get_paired_axis_key() const { return paired_axis_key; }

    /**
     * Returns either the virtual key if it exists, otherwise the regular key value
     */
    [[nodiscard]] constexpr const Key& get_virtual_key() const { return virtual_key ? virtual_key_value : key; }

    constexpr void populate_flags(const KeyFlags flags)
    {
        modifier_key = (flags & KeyFlagsBits::ModifierKey) != KeyFlagsBits::Empty;
        gamepad_key = (flags & KeyFlagsBits::GamepadKey) != KeyFlagsBits::Empty;
        touch = (flags & KeyFlagsBits::Touch) != KeyFlagsBits::Empty;
        mouse_button = (flags & KeyFlagsBits::MouseButton) != KeyFlagsBits::Empty;
        update_axis_without_samples = (flags & KeyFlagsBits::UpdateAxisWithoutSamples) != KeyFlagsBits::Empty;
        virtual_key = (flags & KeyFlagsBits::Virtual) != KeyFlagsBits::Empty;

        if (flags & KeyFlagsBits::ButtonAxis)
        {
            axis_type = AxisType::Button;
        }
        else if (flags & KeyFlagsBits::Axis1D)
        {
            axis_type = AxisType::Axis1D;
        }
        else if (flags & KeyFlagsBits::Axis2D)
        {
            axis_type = AxisType::Axis2D;
        }
        else if (flags & KeyFlagsBits::Axis3D)
        {
            axis_type = AxisType::Axis3D;
        }
    }

private:
    enum class AxisType
    {
        None,
        Button,
        Axis1D,
        Axis2D,
        Axis3D
    };

private:
    Key key;

    PairedAxis axis = PairedAxis::Unpaired;
    Key paired_axis_key = Key::Invalid;

    Key virtual_key_value = Key::Invalid;

    bool modifier_key                : 1 = false;
    bool gamepad_key                 : 1 = false;
    bool touch                       : 1 = false;
    bool mouse_button                : 1 = false;
    bool update_axis_without_samples : 1 = false;
    bool virtual_key                 : 1 = false;
    AxisType axis_type = AxisType::None;

    StringId short_description;
    StringId long_description;
};
} // portal

