//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "editor_theme.h"

#include <glm/gtc/type_ptr.hpp>

namespace portal::imgui
{
namespace
{
    ImVec4 rgb(const float r, const float g, const float b, const float a = 255.f)
    {
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
    }
}

EditorTheme::EditorTheme()
{
    load_default_dark();
}

void EditorTheme::load_default_dark()
{
    colors[ThemeColors::Primary1] = rgb(77, 77, 79);            // UI highlights
    colors[ThemeColors::Primary2] = rgb(70, 70, 77);            // Hover backgrounds
    colors[ThemeColors::Primary3] = rgb(30, 30, 30);            // Panel backgrounds
    colors[ThemeColors::Secondary1] = rgb(20, 20, 20);          // Window background
    colors[ThemeColors::Secondary2] = rgb(55, 55, 61);          // Inactive UI areas
    colors[ThemeColors::AccentPrimaryLeft] = rgb(66, 150, 250); // Main accent left
    colors[ThemeColors::AccentPrimaryRight] = rgb(66, 0, 144);  // Main accent right
    colors[ThemeColors::Accent2] = rgb(96, 115, 181);           // Minor accents
    colors[ThemeColors::Text] = rgb(255, 255, 255);            // Main text
    colors[ThemeColors::TextDarker] = rgb(128, 128, 128);            // Disabled/secondary text
    colors[ThemeColors::Background1] = rgb(37, 37, 38);         // Window background
    colors[ThemeColors::Background2] = rgb(30, 30, 30);         // Group panels
    colors[ThemeColors::Background3] = rgb(51, 51, 51);         // Inner panels
    colors[ThemeColors::Background4] = rgb(0, 0, 0);            // Transparent
    colors[ThemeColors::Error] = rgb(219, 72, 115);             // Errors
    colors[ThemeColors::Warning] = rgb(213, 152, 87);           // Warnings
    colors[ThemeColors::Success] = rgb(174, 243, 87);           // Success
    colors[ThemeColors::X] = rgb(219, 72, 115);                 // Transform axis X
    colors[ThemeColors::Y] = rgb(174, 243, 87);                 // Transform axis Y
    colors[ThemeColors::Z] = rgb(118, 162, 250);                // Transform axis Z
    apply_to_imgui();
}

void EditorTheme::load_default_light()
{
    colors[ThemeColors::Primary1] = rgb(180, 180, 185);         // UI highlights
    colors[ThemeColors::Primary2] = rgb(160, 160, 170);         // Hover backgrounds
    colors[ThemeColors::Primary3] = rgb(210, 210, 210);         // Panel backgrounds
    colors[ThemeColors::Secondary1] = rgb(225, 225, 225);       // Window background
    colors[ThemeColors::Secondary2] = rgb(190, 190, 200);       // Inactive UI areas
    colors[ThemeColors::AccentPrimaryLeft] = rgb(90, 140, 200); // Main accent left
    colors[ThemeColors::AccentPrimaryRight] = rgb(54, 0, 118);  // Main accent right
    colors[ThemeColors::Accent2] = rgb(110, 110, 120);          // Minor accents
    colors[ThemeColors::Text] = rgb(30, 30, 30);               // Main text
    colors[ThemeColors::TextDarker] = rgb(90, 90, 90);               // Disabled/secondary text
    colors[ThemeColors::Background1] = rgb(240, 240, 240);      // Window background
    colors[ThemeColors::Background2] = rgb(225, 225, 225);      // Group panels
    colors[ThemeColors::Background3] = rgb(200, 200, 200);      // Inner panels
    colors[ThemeColors::Background4] = rgb(255, 255, 255);      // Transparent
    colors[ThemeColors::Error] = rgb(219, 72, 115);             // Errors
    colors[ThemeColors::Warning] = rgb(213, 152, 87);           // Warnings
    colors[ThemeColors::Success] = rgb(174, 243, 87);           // Success
    colors[ThemeColors::X] = rgb(219, 72, 115);                 // Transform axis X
    colors[ThemeColors::Y] = rgb(174, 243, 87);                 // Transform axis Y
    colors[ThemeColors::Z] = rgb(118, 162, 250);                // Transform axis Z
    apply_to_imgui();
}

void EditorTheme::load_from_file(std::filesystem::path)
{
    // TODO: implement
}

void EditorTheme::save_to_file(std::filesystem::path)
{
    // TODO: implement
}

void EditorTheme::push_color(const ImGuiCol widget, const ThemeColors color, const float alpha) const
{
    auto theme_color = colors.at(color);
    theme_color.w = alpha;
    ImGui::PushStyleColor(widget, theme_color);
}

void EditorTheme::pop_color(size_t count) const
{
    ImGui::PopStyleColor(static_cast<int>(count));
}

ScopedColor EditorTheme::scoped_color(const ImGuiCol widget, const ThemeColors color, const float alpha) const
{
    auto theme_color = colors.at(color);
    theme_color.w = alpha;
    return ScopedColor(widget, theme_color);
}

ImVec4& EditorTheme::operator[](const ThemeColors color)
{
    return colors[color];
}

const ImVec4& EditorTheme::operator[](const ThemeColors color) const
{
    return colors.at(color);
}

void EditorTheme::show_color_picker()
{
    ScopedWindow window("Theme Color Picker");
    bool changed = false;

    for (auto& [name, color] : colors)
    {
        auto* ptr = &color.x;
        changed |= ImGui::ColorEdit4(enchantum::to_string(name).data(), ptr);
    }
    if (changed)
    {
        apply_to_imgui();
    }
}

void EditorTheme::apply_to_imgui()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_WindowBg] = colors[ThemeColors::Background1];
    style.Colors[ImGuiCol_PopupBg] = colors[ThemeColors::Background2];
    style.Colors[ImGuiCol_Border] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_Header] = colors[ThemeColors::Primary3];
    style.Colors[ImGuiCol_HeaderHovered] = colors[ThemeColors::Primary2];
    style.Colors[ImGuiCol_HeaderActive] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_Button] = colors[ThemeColors::Primary3];
    style.Colors[ImGuiCol_ButtonHovered] = colors[ThemeColors::Primary1];
    style.Colors[ImGuiCol_ButtonActive] = colors[ThemeColors::Primary2];
    style.Colors[ImGuiCol_CheckMark] = colors[ThemeColors::Text];
    style.Colors[ImGuiCol_SliderGrab] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_SliderGrabActive] = colors[ThemeColors::AccentPrimaryLeft];
    style.Colors[ImGuiCol_FrameBg] = colors[ThemeColors::Primary3];
    style.Colors[ImGuiCol_FrameBgHovered] = colors[ThemeColors::Primary1];
    style.Colors[ImGuiCol_FrameBgActive] = colors[ThemeColors::Primary2];
    style.Colors[ImGuiCol_Tab] = colors[ThemeColors::Background2];
    style.Colors[ImGuiCol_TabHovered] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_TabActive] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_TabSelectedOverline] = colors[ThemeColors::AccentPrimaryLeft];
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = colors[ThemeColors::Primary1];
    style.Colors[ImGuiCol_TabUnfocused] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_TabUnfocusedActive] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_TableRowBg] = colors[ThemeColors::Background2];
    style.Colors[ImGuiCol_TableRowBgAlt] = colors[ThemeColors::Background1];
    style.Colors[ImGuiCol_TitleBg] = colors[ThemeColors::Background2];
    style.Colors[ImGuiCol_TitleBgActive] = colors[ThemeColors::Background2];
    style.Colors[ImGuiCol_TitleBgCollapsed] = colors[ThemeColors::Background2];
    style.Colors[ImGuiCol_ScrollbarGrab] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_ResizeGrip] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_ResizeGripHovered] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_ResizeGripActive] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_Separator] = colors[ThemeColors::Primary2];
    style.Colors[ImGuiCol_SeparatorHovered] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_SeparatorActive] = colors[ThemeColors::Secondary2];
    style.Colors[ImGuiCol_Text] = colors[ThemeColors::Text];
    style.Colors[ImGuiCol_TextDisabled] = colors[ThemeColors::TextDarker];
    style.Colors[ImGuiCol_MenuBarBg] = colors[ThemeColors::Secondary1];
}
} // portal
