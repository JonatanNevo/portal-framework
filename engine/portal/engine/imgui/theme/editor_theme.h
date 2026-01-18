//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include <imgui.h>
#include <enchantum/enchantum.hpp>

#include "portal/engine/imgui/imgui_scoped.h"

namespace portal::imgui
{

enum class ThemeColors
{
    Primary1,
    Primary2,
    Primary3,
    Secondary1,
    Secondary2,
    Accent1,
    Accent2,
    Text1,
    Text2,
    Background1,
    Background2,
    Background3,
    Background4,
    Error,
    Warning,
    Success,
    X,
    Y,
    Z,
};

class EditorTheme
{
public:
    EditorTheme();

    void load_default_dark();
    void load_default_light();

    void load_from_file(std::filesystem::path path);
    void save_to_file(std::filesystem::path path);

    void push_color(ImGuiCol widget, ThemeColors color, float alpha = 1.f) const;
    void pop_color(size_t count = 1) const;

    ScopedColor scoped_color(ImGuiCol widget, ThemeColors color, float alpha = 1.f) const;

    ImVec4& operator[](ThemeColors color);
    const ImVec4& operator[](ThemeColors color) const;

    void show_color_picker();

private:
    std::unordered_map<ThemeColors, ImVec4> colors;

    void apply_to_imgui();
};

}
