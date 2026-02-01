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

ImRect rect_offset(const ImRect& rect, const ImVec2 offset)
{
    return rect_offset(rect, offset.x, offset.y);
}

ImRect rect_offset(const ImRect& rect, const float x, const float y)
{
    ImRect result = rect;
    result.Min.x += x;
    result.Min.y += y;
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

void draw_border(const ImRect rect, const float thickness, const float rounding, const ImVec2 offset)
{
    auto min = rect.Min;
    min.x -= thickness;
    min.y -= thickness;
    min.x += offset.x;
    min.y += offset.y;

    auto max = rect.Max;
    max.x += thickness;
    max.y += thickness;
    max.x += offset.x;
    max.y += offset.y;

    auto* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Border)), rounding, 0, thickness);
}

ImColor color_with_multiplied_value(const ImColor& color, const float multiplier)
{
    const ImVec4& raw_value = color.Value;
    float hue, sat, val;
    ImGui::ColorConvertRGBtoHSV(raw_value.x, raw_value.y, raw_value.z, hue, sat, val);
    return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
}

void draw_button_image(
    const vk::DescriptorSet image,
    const ImColor tint_normal,
    const ImColor tint_hovered,
    const ImColor tint_pressed,
    const ImVec2 uv0,
    const ImVec2 uv1
)
{
    return draw_button_image(
        image,
        image,
        image,
        tint_normal,
        tint_hovered,
        tint_pressed,
        ImGui::GetItemRectMin(),
        ImGui::GetItemRectMax(),
        uv0,
        uv1
    );
}

void draw_button_image(
    const vk::DescriptorSet image,
    const ImColor tint_normal,
    const ImColor tint_hovered,
    const ImColor tint_pressed,
    const ImRect rect,
    const ImVec2 uv0,
    const ImVec2 uv1
)
{
    return draw_button_image(image, image, image, tint_normal, tint_hovered, tint_pressed, rect.Min, rect.Max, uv0, uv1);
}

void draw_button_image(
    const vk::DescriptorSet image,
    const ImColor tint_normal,
    const ImColor tint_hovered,
    const ImColor tint_pressed,
    const ImVec2 rect_min,
    const ImVec2 rect_max,
    const ImVec2 uv0,
    const ImVec2 uv1
)
{
    return draw_button_image(image, image, image, tint_normal, tint_hovered, tint_pressed, rect_min, rect_max, uv0, uv1);
}

void draw_button_image(
    const vk::DescriptorSet image_normal,
    const vk::DescriptorSet image_hovered,
    const vk::DescriptorSet image_pressed,
    const ImColor tint_normal,
    const ImColor tint_hovered,
    const ImColor tint_pressed,
    const ImVec2 rect_min,
    const ImVec2 rect_max,
    const ImVec2 uv0,
    const ImVec2 uv1
)
{
    auto* draw_list = ImGui::GetWindowDrawList();
    if (ImGui::IsItemActive())
        draw_list->AddImage(static_cast<VkDescriptorSet>(image_pressed), rect_min, rect_max, uv0, uv1, tint_pressed);
    else if (ImGui::IsItemHovered())
        draw_list->AddImage(static_cast<VkDescriptorSet>(image_hovered), rect_min, rect_max, uv0, uv1, tint_hovered);
    else
        draw_list->AddImage(static_cast<VkDescriptorSet>(image_normal), rect_min, rect_max, uv0, uv1, tint_normal);
}
}
