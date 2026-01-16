//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "imgui_fonts.h"

namespace portal
{
static auto logger = Log::get_logger("ImGui");

void ImGuiFonts::add(ImGuiFontConfiguration font, const bool is_default, const bool merge_with_last)
{
    auto font_reference = font.font;
    const auto& [_, path, glyph_range] = font_reference->get_properties();
    if (fonts.contains(font.name))
    {
        LOGGER_WARN("Tried adding the font {} more than once", font.name);
        return;
    }

    ImFontConfig config{};
    config.MergeMode = merge_with_last;
    auto& io = ImGui::GetIO();

    const ImWchar* im_glyph_range = nullptr;

    if (glyph_range.has_value())
    {
        static_assert(sizeof(ImWchar) == sizeof(wchar_t), "Cannot convert font glyph range to ImWchar");
        im_glyph_range = reinterpret_cast<const ImWchar*>(glyph_range.value().data());
    }

    ImFont* im_font = io.Fonts->AddFontFromFileTTF(
        path.string().c_str(),
        font.size,
        &config,
        im_glyph_range
    );
    PORTAL_ASSERT(im_font, "Failed to load font from file: {}", path.generic_string());
    fonts[font.name] = im_font;

    if (is_default)
        io.FontDefault = im_font;
}

void ImGuiFonts::push_font(const StringId& font_name)
{
    if (!fonts.contains(font_name))
    {
        const auto& io = ImGui::GetIO();
        ImGui::PushFont(io.FontDefault);
        return;
    }

    ImGui::PushFont(fonts[font_name]);
}


void ImGuiFonts::pop_font()
{
    ImGui::PopFont();
}

ImFont* ImGuiFonts::get(const StringId& font_name)
{
    PORTAL_ASSERT(fonts.contains(font_name), "Font {} not found", font_name);
    return fonts[font_name];
}
} // portal
