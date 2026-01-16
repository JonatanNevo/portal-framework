//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <imgui.h>

namespace portal::imgui
{
class ScopedStyle
{
public:
    ScopedStyle(const ScopedStyle&) = delete;
    ScopedStyle& operator=(const ScopedStyle&) = delete;

    template <typename T>
    ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }

    ~ScopedStyle() { ImGui::PopStyleVar(); }
};

class ScopedColor
{
public:
    ScopedColor(const ScopedColor&) = delete;
    ScopedColor& operator=(const ScopedColor&) = delete;

    ScopedColor(ImGuiCol colorVar, const ImVec4& value) { ImGui::PushStyleColor(colorVar, value); }
    ScopedColor(ImGuiCol colorVar, ImU32 value) { ImGui::PushStyleColor(colorVar, value); }

    ~ScopedColor() { ImGui::PopStyleColor(); }
};

bool is_item_hovered(float delay_in_seconds = 0.1f, ImGuiHoveredFlags flags = ImGuiHoveredFlags_None);

void set_tooltip(std::string_view tooltip, float delay_in_seconds = 0.1f, bool allow_when_disabled = true, ImVec2 padding = ImVec2{5.f, 5.f});
}
