//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
// This code was originally  taken from hazel engine: https://docs.hazelengine.com/
// Licensed under - Apache License 2.0
//

#pragma once
#include <imgui.h>

// To experiment with editor theme live you can change these constexpr into static
// members of a static "Theme" class and add a quick ImGui window to adjust the color values
namespace portal::colors
{

struct DefaultTheme
{
    static constexpr auto accent = IM_COL32(236, 158, 36, 255);
    static constexpr auto accent_brighter = IM_COL32(255, 200, 100, 255);
    static constexpr auto highlight = IM_COL32(39, 185, 242, 255);
    static constexpr auto nice_blue = IM_COL32(83, 232, 254, 255);
    static constexpr auto compliment = IM_COL32(78, 151, 166, 255);
    static constexpr auto background = IM_COL32(36, 36, 36, 255);
    static constexpr auto background_dark = IM_COL32(26, 26, 26, 255);
    static constexpr auto titlebar = IM_COL32(21, 21, 21, 255);
    static constexpr auto titlebar_orange = IM_COL32(186, 66, 30, 255);
    static constexpr auto titlebar_green = IM_COL32(18, 88, 30, 255);
    static constexpr auto titlebar_red = IM_COL32(185, 30, 30, 255);
    static constexpr auto property_field = IM_COL32(15, 15, 15, 255);
    static constexpr auto text = IM_COL32(192, 192, 192, 255);
    static constexpr auto text_brighter = IM_COL32(210, 210, 210, 255);
    static constexpr auto text_darker = IM_COL32(128, 128, 128, 255);
    static constexpr auto text_error = IM_COL32(230, 51, 51, 255);
    static constexpr auto muted = IM_COL32(77, 77, 77, 255);
    static constexpr auto group_header = IM_COL32(47, 47, 47, 255);
    static constexpr auto selection = IM_COL32(237, 192, 119, 255);
    static constexpr auto selection_muted = IM_COL32(237, 201, 142, 23);
    static constexpr auto background_popup = IM_COL32(50, 50, 50, 255);
    static constexpr auto valid_prefab = IM_COL32(82, 179, 222, 255);
    static constexpr auto invalid_prefab = IM_COL32(222, 43, 43, 255);
    static constexpr auto missing_mesh = IM_COL32(230, 102, 76, 255);
    static constexpr auto mesh_not_set = IM_COL32(250, 101, 23, 255);
    static constexpr auto server_domain = IM_COL32(82, 222, 150, 255);
};

using Theme = DefaultTheme;

}
