//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <imgui.h>
#include <string.h>

#include "portal/engine/editor/editor_context.h"
#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/engine/imgui/utils.h"

namespace portal::imgui
{

struct SearchWidgetConsts
{
    float widget_cursor_shift = 1.f;
    float frame_rounding = 3.f;
    float frame_padding_x = 28.f;
    float same_line_offset = 5.f;
    float icon_padding_y_offset = 3.f;
    float hint_padding_y_offset = 1.f;
    float clear_icon_spacing_x = 4.f;
    float clear_icon_rect_expand = 2.f;
};

template <size_t buffer_size, typename StringType>
bool search_widget(EditorContext& context, StringType& search_string, const char* hint = "Search...", bool* grab_focus = nullptr)
{
    constexpr SearchWidgetConsts consts;

    push_id();

    shift_cursor(0.f, consts.widget_cursor_shift);

    bool layout_suspended = false;
    {
        const ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->DC.CurrentLayout)
        {
            ImGui::SuspendLayout();
            layout_suspended = true;
        }
    }

    bool modified = false;
    bool searching = false;

    const float area_pos_x = ImGui::GetCursorPosX();
    const float frame_padding_y = ImGui::GetStyle().FramePadding.y;

    ScopedStyle rounding(ImGuiStyleVar_FrameRounding, consts.frame_rounding);
    ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(consts.frame_padding_x, frame_padding_y));

    if constexpr (std::is_same_v<StringType, std::string>)
    {
        std::array<char, buffer_size + 1> search_buffer{};
        strncpy_s(search_buffer.data(), search_string.c_str(), buffer_size);
        if (ImGui::InputText(generate_id(), search_buffer.data(), buffer_size))
        {
            search_string = search_buffer.data();
            modified = true;
        }
        else if (ImGui::IsItemDeactivatedAfterEdit())
        {
            search_string = search_buffer.data();
            modified = true;
        }
    }
    else
    {
        static_assert(std::is_same_v<decltype(&search_string[0]), char*>, "search_string paramenter must be std::string& or char*");

        if (ImGui::InputText(generate_id(), search_string, buffer_size))
        {
            modified = true;
        }
        else if (ImGui::IsItemDeactivatedAfterEdit())
        {
            modified = true;
        }
    }

    searching = search_string[0] != '\0';

    if (grab_focus && *grab_focus)
    {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(
            ImGuiMouseButton_Left
        ))
        {
            ImGui::SetKeyboardFocusHere(-1);
        }

        if (ImGui::IsItemFocused())
        {
            *grab_focus = false;
        }
    }

    ImGui::SetNextItemAllowOverlap();
    draw_item_activity_outline();

    ImGui::SameLine(area_pos_x + consts.same_line_offset);

    if (layout_suspended)
        ImGui::ResumeLayout();

    ImGui::BeginHorizontal(generate_id(), ImGui::GetItemRectSize());
    const ImVec2 icon_size(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight());

    // Search Icon
    {
        const float icon_y_offset = frame_padding_y - consts.icon_padding_y_offset;
        shift_cursor(0.f, icon_y_offset);
        ImGui::Image(
            static_cast<VkDescriptorSet>(context.icons.get_descriptor(EditorIcon::Search)),
            icon_size,
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImVec4(1.0f, 1.0f, 1.0f, 0.2f),
            ImVec4(0, 0, 0, 0)
        );
        shift_cursor(0.f, -icon_y_offset);
    }

    // Hint
    if (!searching)
    {
        shift_cursor(0.f, -frame_padding_y + consts.hint_padding_y_offset);
        auto text = context.theme.scoped_color(ImGuiCol_Text, ThemeColors::TextDarker);
        ScopedStyle text_padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, frame_padding_y));
        ImGui::TextUnformatted(hint);
        shift_cursor(0.f, frame_padding_y - consts.hint_padding_y_offset);
    }

    ImGui::Spring();

    // Clear icon
    if (searching)
    {
        const float line_height = ImGui::GetItemRectSize().y - frame_padding_y / 2.f;

        if (ImGui::InvisibleButton(generate_id(), ImVec2{line_height, line_height}))
        {
            if constexpr (std::is_same_v<StringType, std::string>)
            {
                search_string.clear();
            }
            else
            {
                memset(search_string, 0, buffer_size);
            }

            modified = true;
        }

        if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        }

        const auto text = context.theme[imgui::ThemeColors::Text];
        draw_button_image(
        context.icons.get_descriptor(EditorIcon::Clear),
            text,
            color_with_multiplied_value(text, 1.2f),
            color_with_multiplied_value(text, 0.8f),
            expand_rect(get_item_rect(), -consts.clear_icon_rect_expand, -consts.clear_icon_rect_expand)
        );

        ImGui::Spring(-1.0f, consts.clear_icon_spacing_x * 2.0f);
    }

    ImGui::EndHorizontal();
    shift_cursor(0.f, -consts.widget_cursor_shift);
    pop_id();
    return modified;
}
}
