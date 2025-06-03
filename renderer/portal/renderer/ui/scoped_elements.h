//
// Created by Jonatan Nevo on 11/02/2025.
//

#pragma once

#include <utility>


#include "imgui.h"
#include "imgui_internal.h"

namespace portal::ui
{
class ScopedStyle
{
public:
    ScopedStyle(const ScopedStyle&) = delete;
    ScopedStyle operator=(const ScopedStyle&) = delete;

    template <typename T>
    ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }

    ~ScopedStyle() { ImGui::PopStyleVar(); }
};

class ScopedColor
{
public:
    ScopedColor(const ScopedColor&) = delete;
    ScopedColor operator=(const ScopedColor&) = delete;

    template <typename T>
    ScopedColor(ImGuiCol ColorId, T Color) { ImGui::PushStyleColor(ColorId, Color); }

    ~ScopedColor() { ImGui::PopStyleColor(); }
};

class ScopedFont
{
public:
    ScopedFont(const ScopedFont&) = delete;
    ScopedFont operator=(const ScopedFont&) = delete;

    ScopedFont(ImFont* font) { ImGui::PushFont(font); }
    ~ScopedFont() { ImGui::PopFont(); }
};

class ScopedID
{
public:
    ScopedID(const ScopedID&) = delete;
    ScopedID operator=(const ScopedID&) = delete;

    template <typename T>
    ScopedID(T id) { ImGui::PushID(id); }

    ~ScopedID() { ImGui::PopID(); }
};

class ScopedColorStack
{
public:
    ScopedColorStack(const ScopedColorStack&) = delete;
    ScopedColorStack operator=(const ScopedColorStack&) = delete;

    template <typename ColorType, typename... OtherColors>
    ScopedColorStack(ImGuiCol firstColorID, ColorType firstColor, OtherColors&&... otherColorPairs) :
        count((sizeof...(otherColorPairs) / 2) + 1)
    {
        static_assert(
            (sizeof...(otherColorPairs) & 1u) == 0,
            "ScopedColorStack constructor expects a list of pairs of Color IDs and Colors as its arguments"
            );

        push_color(firstColorID, firstColor, std::forward<OtherColors>(otherColorPairs)...);
    }

    ~ScopedColorStack() { ImGui::PopStyleColor(count); }

private:
    int count;

    template <typename ColorType, typename... OtherColors>
    void push_color(ImGuiCol ColorID, ColorType Color, OtherColors&&... otherColorPairs)
    {
        if constexpr (sizeof...(otherColorPairs) == 0)
        {
            ImGui::PushStyleColor(ColorID, Color);
        }
        else
        {
            ImGui::PushStyleColor(ColorID, Color);
            PushColor(std::forward<OtherColors>(otherColorPairs)...);
        }
    }
};

class ScopedStyleStack
{
public:
    ScopedStyleStack(const ScopedStyleStack&) = delete;
    ScopedStyleStack operator=(const ScopedStyleStack&) = delete;

    template <typename ValueType, typename... OtherStylePairs>
    ScopedStyleStack(ImGuiStyleVar firstStyleVar, ValueType firstValue, OtherStylePairs&&... otherStylePairs) :
        count((sizeof...(otherStylePairs) / 2) + 1)
    {
        static_assert(
            (sizeof...(otherStylePairs) & 1u) == 0,
            "ScopedStyleStack constructor expects a list of pairs of Color IDs and Colors as its arguments"
            );

        PushStyle(firstStyleVar, firstValue, std::forward<OtherStylePairs>(otherStylePairs)...);
    }

    ~ScopedStyleStack() { ImGui::PopStyleVar(count); }

private:
    int count;

    template <typename ValueType, typename... OtherStylePairs>
    void push_style(ImGuiStyleVar styleVar, ValueType value, OtherStylePairs&&... otherStylePairs)
    {
        if constexpr (sizeof...(otherStylePairs) == 0)
        {
            ImGui::PushStyleVar(styleVar, value);
        }
        else
        {
            ImGui::PushStyleVar(styleVar, value);
            PushStyle(std::forward<OtherStylePairs>(otherStylePairs)...);
        }
    }
};

class ScopedItemFlags
{
public:
    ScopedItemFlags(const ScopedItemFlags&) = delete;
    ScopedItemFlags operator=(const ScopedItemFlags&) = delete;

    ScopedItemFlags(const ImGuiItemFlags flags, const bool enable = true) { ImGui::PushItemFlag(flags, enable); }
    ~ScopedItemFlags() { ImGui::PopItemFlag(); }
};
}
