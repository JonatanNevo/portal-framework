//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/editor/editor_context.h"

namespace portal::imgui
{
template <typename Func>
void confirm_and_execute(bool& should_execute, const char* title, const char* message, Func&& on_confirm, const EditorContext& context)
{
    if (!should_execute)
        return;

    ImGui::OpenPopup(title);
    imgui::ScopedStyle window_padding(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

    ImGui::SetNextWindowSizeConstraints({300.0f, 0.0f}, {400.0f, std::numeric_limits<float>::max()});
    auto popup_background = context.theme.scoped_color(ImGuiCol_PopupBg, imgui::ThemeColors::Background1);

    {
        imgui::ScopedPopupModal popup(title);
        if (popup.is_open)
        {
            {
                auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Warning);
                ImGui::TextWrapped("%s", message);
            }

            float width = ImGui::GetContentRegionAvail().x;
            float button_width = width * 0.5f - 5.f;
            ImGui::Dummy({5.f, 0.f});

            auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Primary3);
            if (ImGui::Button("Yes", {button_width, 0.f}))
            {
                on_confirm();
                should_execute = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("No", {button_width, 0.f}))
            {
                should_execute = false;
                ImGui::CloseCurrentPopup();
            }
        }
    }
}
}
