//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "options_button.h"

#include <algorithm>
#include <imgui.h>

#include "portal/engine/editor/editor_context.h"

namespace portal::imgui
{
bool options_button(EditorContext& context)
{
    const bool clicked = ImGui::InvisibleButton("##options", ImVec2{ImGui::GetFrameHeight(), ImGui::GetFrameHeight()});

    const float space_available = std::min(ImGui::GetItemRectSize().x, ImGui::GetItemRectSize().y);
    constexpr float desired_icon_size = 15.f;
    const float padding = std::max((space_available - desired_icon_size) / 2.f, 0.f);

    const auto button_color = context.theme[ThemeColors::Text];
    const uint8_t single_value = static_cast<uint8_t>(ImColor(button_color).Value.x * 255);
    draw_button_image(
        context.icons.get_descriptor(EditorIcon::Settings),
        IM_COL32(single_value, single_value, single_value, 200),
        IM_COL32(single_value, single_value, single_value, 255),
        IM_COL32(single_value, single_value, single_value, 150),
        expand_rect(get_item_rect(), -padding, -padding)
    );

    return clicked;
}
}
