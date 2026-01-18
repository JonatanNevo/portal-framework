//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/engine/imgui/theme/serializers.h"

namespace portal
{
void Archivable<ImVec4>::archive(const ImVec4& vec, ArchiveObject& archive)
{
    archive.add_property("x", vec.x);
    archive.add_property("y", vec.y);
    archive.add_property("z", vec.z);
    archive.add_property("w", vec.w);
}

ImVec4 Archivable<ImVec4>::dearchive(ArchiveObject& archive)
{
    ImVec4 result;
    archive.get_property("x", result.x);
    archive.get_property("y", result.y);
    archive.get_property("z", result.z);
    archive.get_property("w", result.w);
    return result;
}

void Archivable<ImVec2>::archive(const ImVec2& vec, ArchiveObject& archive)
{
    archive.add_property("x", vec.x);
    archive.add_property("y", vec.y);
}

ImVec2 Archivable<ImVec2>::dearchive(ArchiveObject& archive)
{
    ImVec2 result;
    archive.get_property("x", result.x);
    archive.get_property("y", result.y);
    return result;
}

void Archivable<ImGuiStyle>::archive(const ImGuiStyle& style, ArchiveObject& archive)
{
    archive.add_property("Alpha", style.Alpha);
    archive.add_property("DisabledAlpha", style.DisabledAlpha);
    archive.add_property("WindowPadding", style.WindowPadding);
    archive.add_property("WindowRounding", style.WindowRounding);
    archive.add_property("WindowBorderSize", style.WindowBorderSize);
    archive.add_property("WindowBorderHoverPadding", style.WindowBorderHoverPadding);
    archive.add_property("WindowMinSize", style.WindowMinSize);
    archive.add_property("WindowTitleAlign", style.WindowTitleAlign);
    archive.add_property("WindowMenuButtonPosition", style.WindowMenuButtonPosition);
    archive.add_property("ChildRounding", style.ChildRounding);
    archive.add_property("ChildBorderSize", style.ChildBorderSize);
    archive.add_property("PopupRounding", style.PopupRounding);
    archive.add_property("PopupBorderSize", style.PopupBorderSize);
    archive.add_property("FramePadding", style.FramePadding);
    archive.add_property("FrameRounding", style.FrameRounding);
    archive.add_property("FrameBorderSize", style.FrameBorderSize);
    archive.add_property("ItemSpacing", style.ItemSpacing);
    archive.add_property("ItemInnerSpacing", style.ItemInnerSpacing);
    archive.add_property("CellPadding", style.CellPadding);
    archive.add_property("TouchExtraPadding", style.TouchExtraPadding);
    archive.add_property("IndentSpacing", style.IndentSpacing);
    archive.add_property("ColumnsMinSpacing", style.ColumnsMinSpacing);
    archive.add_property("ScrollbarSize", style.ScrollbarSize);
    archive.add_property("ScrollbarRounding", style.ScrollbarRounding);
    archive.add_property("GrabMinSize", style.GrabMinSize);
    archive.add_property("GrabRounding", style.GrabRounding);
    archive.add_property("LayoutAlign", style.LayoutAlign);
    archive.add_property("LogSliderDeadzone", style.LogSliderDeadzone);
    archive.add_property("ImageBorderSize", style.ImageBorderSize);
    archive.add_property("TabRounding", style.TabRounding);
    archive.add_property("TabBorderSize", style.TabBorderSize);
    archive.add_property("TabCloseButtonMinWidthSelected", style.TabCloseButtonMinWidthSelected);
    archive.add_property("TabCloseButtonMinWidthUnselected", style.TabCloseButtonMinWidthUnselected);
    archive.add_property("TabBarBorderSize", style.TabBarBorderSize);
    archive.add_property("TabBarOverlineSize", style.TabBarOverlineSize);
    archive.add_property("TableAngledHeadersAngle", style.TableAngledHeadersAngle);
    archive.add_property("TableAngledHeadersTextAlign", style.TableAngledHeadersTextAlign);
    archive.add_property("ColorButtonPosition", style.ColorButtonPosition);
    archive.add_property("ButtonTextAlign", style.ButtonTextAlign);
    archive.add_property("SelectableTextAlign", style.SelectableTextAlign);
    archive.add_property("SeparatorTextBorderSize", style.SeparatorTextBorderSize);
    archive.add_property("SeparatorTextAlign", style.SeparatorTextAlign);
    archive.add_property("SeparatorTextPadding", style.SeparatorTextPadding);
    archive.add_property("DisplayWindowPadding", style.DisplayWindowPadding);
    archive.add_property("DisplaySafeAreaPadding", style.DisplaySafeAreaPadding);
    archive.add_property("DockingSeparatorSize", style.DockingSeparatorSize);
    archive.add_property("MouseCursorScale", style.MouseCursorScale);
    archive.add_property("AntiAliasedLines", style.AntiAliasedLines);
    archive.add_property("AntiAliasedLinesUseTex", style.AntiAliasedLinesUseTex);
    archive.add_property("AntiAliasedFill", style.AntiAliasedFill);
    archive.add_property("CurveTessellationTol", style.CurveTessellationTol);
    archive.add_property("CircleTessellationMaxError", style.CircleTessellationMaxError);
    archive.add_property("HoverStationaryDelay", style.HoverStationaryDelay);
    archive.add_property("HoverDelayShort", style.HoverDelayShort);
    archive.add_property("HoverDelayNormal", style.HoverDelayNormal);
    archive.add_property("HoverFlagsForTooltipMouse", style.HoverFlagsForTooltipMouse);
    archive.add_property("HoverFlagsForTooltipNav", style.HoverFlagsForTooltipNav);

    std::vector<ImVec4> colors(ImGuiCol_COUNT);
    std::memcpy(colors.data(), style.Colors, sizeof(ImVec4) * ImGuiCol_COUNT);
    archive.add_property("Colors", colors);
}

ImGuiStyle Archivable<ImGuiStyle>::dearchive(ArchiveObject& archive)
{
    ImGuiStyle style;
    std::vector<ImVec4> colors(ImGuiCol_COUNT);
    archive.get_property("Colors", colors);
    std::memcpy(style.Colors, colors.data(), sizeof(ImVec4) * ImGuiCol_COUNT);

    archive.get_property("Alpha", style.Alpha);
    archive.get_property("DisabledAlpha", style.DisabledAlpha);
    archive.get_property("WindowPadding", style.WindowPadding);
    archive.get_property("WindowRounding", style.WindowRounding);
    archive.get_property("WindowBorderSize", style.WindowBorderSize);
    archive.get_property("WindowBorderHoverPadding", style.WindowBorderHoverPadding);
    archive.get_property("WindowMinSize", style.WindowMinSize);
    archive.get_property("WindowTitleAlign", style.WindowTitleAlign);
    archive.get_property("WindowMenuButtonPosition", style.WindowMenuButtonPosition);
    archive.get_property("ChildRounding", style.ChildRounding);
    archive.get_property("ChildBorderSize", style.ChildBorderSize);
    archive.get_property("PopupRounding", style.PopupRounding);
    archive.get_property("PopupBorderSize", style.PopupBorderSize);
    archive.get_property("FramePadding", style.FramePadding);
    archive.get_property("FrameRounding", style.FrameRounding);
    archive.get_property("FrameBorderSize", style.FrameBorderSize);
    archive.get_property("ItemSpacing", style.ItemSpacing);
    archive.get_property("ItemInnerSpacing", style.ItemInnerSpacing);
    archive.get_property("CellPadding", style.CellPadding);
    archive.get_property("TouchExtraPadding", style.TouchExtraPadding);
    archive.get_property("IndentSpacing", style.IndentSpacing);
    archive.get_property("ColumnsMinSpacing", style.ColumnsMinSpacing);
    archive.get_property("ScrollbarSize", style.ScrollbarSize);
    archive.get_property("ScrollbarRounding", style.ScrollbarRounding);
    archive.get_property("GrabMinSize", style.GrabMinSize);
    archive.get_property("GrabRounding", style.GrabRounding);
    archive.get_property("LayoutAlign", style.LayoutAlign);
    archive.get_property("LogSliderDeadzone", style.LogSliderDeadzone);
    archive.get_property("ImageBorderSize", style.ImageBorderSize);
    archive.get_property("TabRounding", style.TabRounding);
    archive.get_property("TabBorderSize", style.TabBorderSize);
    archive.get_property("TabCloseButtonMinWidthSelected", style.TabCloseButtonMinWidthSelected);
    archive.get_property("TabCloseButtonMinWidthUnselected", style.TabCloseButtonMinWidthUnselected);
    archive.get_property("TabBarBorderSize", style.TabBarBorderSize);
    archive.get_property("TabBarOverlineSize", style.TabBarOverlineSize);
    archive.get_property("TableAngledHeadersAngle", style.TableAngledHeadersAngle);
    archive.get_property("TableAngledHeadersTextAlign", style.TableAngledHeadersTextAlign);
    archive.get_property("ColorButtonPosition", style.ColorButtonPosition);
    archive.get_property("ButtonTextAlign", style.ButtonTextAlign);
    archive.get_property("SelectableTextAlign", style.SelectableTextAlign);
    archive.get_property("SeparatorTextBorderSize", style.SeparatorTextBorderSize);
    archive.get_property("SeparatorTextAlign", style.SeparatorTextAlign);
    archive.get_property("SeparatorTextPadding", style.SeparatorTextPadding);
    archive.get_property("DisplayWindowPadding", style.DisplayWindowPadding);
    archive.get_property("DisplaySafeAreaPadding", style.DisplaySafeAreaPadding);
    archive.get_property("DockingSeparatorSize", style.DockingSeparatorSize);
    archive.get_property("MouseCursorScale", style.MouseCursorScale);
    archive.get_property("AntiAliasedLines", style.AntiAliasedLines);
    archive.get_property("AntiAliasedLinesUseTex", style.AntiAliasedLinesUseTex);
    archive.get_property("AntiAliasedFill", style.AntiAliasedFill);
    archive.get_property("CurveTessellationTol", style.CurveTessellationTol);
    archive.get_property("CircleTessellationMaxError", style.CircleTessellationMaxError);
    archive.get_property("HoverStationaryDelay", style.HoverStationaryDelay);
    archive.get_property("HoverDelayShort", style.HoverDelayShort);
    archive.get_property("HoverDelayNormal", style.HoverDelayNormal);
    archive.get_property("HoverFlagsForTooltipMouse", style.HoverFlagsForTooltipMouse);
    archive.get_property("HoverFlagsForTooltipNav", style.HoverFlagsForTooltipNav);
    return style;
}
}
