//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "utils.h"
#include <imgui_internal.h>

namespace portal::imgui
{
bool is_item_hovered(const float delay_in_seconds, const ImGuiHoveredFlags flags)
{
    return ImGui::IsItemHovered(flags) && GImGui->HoveredIdTimer > delay_in_seconds;
}

void set_tooltip(std::string_view tooltip, float delay_in_seconds, bool allow_when_disabled, ImVec2 padding)
{
    if (is_item_hovered(delay_in_seconds, allow_when_disabled ? ImGuiHoveredFlags_AllowWhenDisabled : ImGuiHoveredFlags_None))
    {
        ScopedStyle tooltip_padding(ImGuiStyleVar_WindowPadding, padding);
        ImGui::SetTooltip("%s", tooltip.data());
    }
}
}
