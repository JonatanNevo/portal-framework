//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
// This code was originally taken from hazel engine: https://docs.hazelengine.com/
// Licensed under - Apache License 2.0
//

#pragma once

#include <imgui.h>
#include <imgui_internal.h>

#include "colors.h"
#include "portal/engine/strings/string_id.h"

namespace portal::ImGuiEx
{

//=========================================================================================
/// Utilities

class ScopedStyle
{
public:
    ScopedStyle(const ScopedStyle&) = delete;
    ScopedStyle& operator=(const ScopedStyle&) = delete;

    template <typename T>
    ScopedStyle(ImGuiStyleVar style_var, T value) { ImGui::PushStyleVar(style_var, value); }

    ~ScopedStyle() { ImGui::PopStyleVar(); }
};

class ScopedColor
{
public:
    ScopedColor(const ScopedColor&) = delete;
    ScopedColor& operator=(const ScopedColor&) = delete;

    template <typename T>
    ScopedColor(const ImGuiCol color_id, T color) { ImGui::PushStyleColor(color_id, ImColor(color).Value); }

    ~ScopedColor() { ImGui::PopStyleColor(); }
};

class ScopedFont
{
public:
    ScopedFont(const ScopedFont&) = delete;
    ScopedFont& operator=(const ScopedFont&) = delete;

    ScopedFont(ImFont* font) { ImGui::PushFont(font); }

    ~ScopedFont() { ImGui::PopFont(); }
};

class ScopedID
{
public:
    ScopedID(const ScopedID&) = delete;
    ScopedID& operator=(const ScopedID&) = delete;

    template <typename T>
    ScopedID(T id) { ImGui::PushID(id); }

    ~ScopedID() { ImGui::PopID(); }
};

template <>
inline ScopedID::ScopedID(const StringId id) { ImGui::PushID(reinterpret_cast<const void*>(id.id)); }

// because otherwise it will call PushID(int) which is not what we want

class ScopedColorStack
{
public:
    ScopedColorStack(const ScopedColorStack&) = delete;
    ScopedColorStack& operator=(const ScopedColorStack&) = delete;

    template <typename ColorType, typename... OtherColors>
    ScopedColorStack(ImGuiCol first_colour_id, ColorType first_color, OtherColors&&... other_color_pairs)
        : count((sizeof...(other_color_pairs) / 2) + 1)
    {
        static_assert(
            (sizeof...(other_color_pairs) & 1u) == 0,
            "ScopedColorStack constructor expects a list of pairs of color IDs and colors as its arguments"
            );

        PushColor(first_colour_id, first_color, std::forward<OtherColors>(other_color_pairs)...);
    }

    ~ScopedColorStack() { ImGui::PopStyleColor(count); }

private:
    int count;

    template <typename ColorType, typename... OtherColors>
    void PushColor(const ImGuiCol color_id, ColorType color, OtherColors&&... other_color_pairs)
    {
        if constexpr (sizeof...(other_color_pairs) == 0)
        {
            ImGui::PushStyleColor(color_id, ImColor(color).Value);
        }
        else
        {
            ImGui::PushStyleColor(color_id, ImColor(color).Value);
            PushColor(std::forward<OtherColors>(other_color_pairs)...);
        }
    }
};

// Check if navigated to current item, e.g. with arrow keys
inline bool navigated_to()
{
    ImGuiContext& g = *GImGui;
    return g.NavJustMovedToId == g.LastItemData.ID;
}

//=========================================================================================
/// Cursor

inline void shift_cursor_x(const float distance)
{
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + distance);
}

inline void shift_cursor_y(const float distance)
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + distance);
}

inline void shift_cursor(const float x, const float y)
{
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursor.x + x, cursor.y + y));
}

//=========================================================================================
/// Colors

inline ImColor color_with_multiplied_value(const ImColor& color, const float multiplier)
{
    const ImVec4& col_raw = color.Value;
    float hue, sat, val;
    ImGui::ColorConvertRGBtoHSV(col_raw.x, col_raw.y, col_raw.z, hue, sat, val);
    return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
}

namespace draw
{
    //=========================================================================================
    /// Lines
    inline void underline(const bool full_width = false, const float x_offset = 0.0f, const float y_offset = -1.0f)
    {
        if (full_width)
        {
            if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
                ImGui::PushColumnsBackground();
            else if (ImGui::GetCurrentTable() != nullptr)
                ImGui::TablePushBackgroundChannel();
        }

        const float width = full_width ? ImGui::GetWindowWidth() : ImGui::GetContentRegionAvail().x;
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(cursor.x + x_offset, cursor.y + y_offset),
            ImVec2(cursor.x + width, cursor.y + y_offset),
            colors::Theme::background_dark,
            1.0f
            );

        if (full_width)
        {
            if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
                ImGui::PopColumnsBackground();
            else if (ImGui::GetCurrentTable() != nullptr)
                ImGui::TablePopBackgroundChannel();
        }
    }
}

}
