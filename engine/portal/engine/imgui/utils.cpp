//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "utils.h"
#include <imgui_internal.h>

#include "imgui_scoped.h"

namespace portal::imgui
{
void draw_item_activity_outline(OutlineFlags flags, ImColor color_highlight, float rounding)
{
    if (is_item_disabled())
        return;

    auto* draw_list = ImGui::GetWindowDrawList();
    const auto rect = expand_rect(get_item_rect(), 1.f);
    if (flags & OutlineFlags_WhenActive && ImGui::IsItemActive())
    {
        if (flags & OutlineFlags_HighlightActive)
        {
            draw_list->AddRect(rect.Min, rect.Max, color_highlight, rounding, 0, 1.5f);
        }
        else
        {
            draw_list->AddRect(rect.Min, rect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
        }
    }
    else if (flags & OutlineFlags_WhenHovered && ImGui::IsItemHovered() && !ImGui::IsItemActive())
    {
        draw_list->AddRect(rect.Min, rect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
    }
    else if (flags & OutlineFlags_WhenInactive && !ImGui::IsItemHovered() && !ImGui::IsItemActive())
    {
        draw_list->AddRect(rect.Min, rect.Max, ImColor(50, 50, 50), rounding, 0, 1.0f);
    }
}

ImRect get_item_rect()
{
    return ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
}

ImRect expand_rect(const ImRect& rect, const float size)
{
    return expand_rect(rect, size, size);
}

ImRect expand_rect(const ImRect& rect, const float x, const float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

bool is_item_disabled()
{
    return ImGui::GetItemFlags() & ImGuiItemFlags_Disabled;
}

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

void shift_cursor(const float x, const float y)
{
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursor.x + x, cursor.y + y));
}

void shift_cursor(const ImVec2 vec)
{
    shift_cursor(vec.x, vec.y);
}
}
