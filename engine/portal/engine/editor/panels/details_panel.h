//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "panel.h"
#include <imgui.h>

#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/engine/imgui/dialogs.h"
#include "portal/engine/ecs/entity.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/third_party/font_awsome/IconsFontAwesome6.h"

namespace portal
{
class DetailsPanel : public Panel
{
public:
    void on_gui_render(EditorContext& context, FrameContext& frame) override;

private:
    // TODO: use the same way `std::hash` works and have each component implement some struct in order to facilitate this instead of passing a function
    template <typename T, typename Func>
    static void draw_component(
        EditorContext& context,
        const std::string_view title,
        Entity& entity,
        Func&& draw_func,
        const bool removable = true
    )
    {
        struct DrawComponentConsts
        {
            ImVec2 padding = {3.f, 3.f};
            float icon_padding_scale = 2.f;
            float margin_right = 5.f;
        };

        constexpr ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

        constexpr DrawComponentConsts consts{};

        if (!entity.has_component<T>())
            return;

        static bool delete_component = false;
        imgui::ScopedID id(title.data());

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, consts.padding);

        auto frame_background = context.theme.scoped_color(ImGuiCol_FrameBg, imgui::ThemeColors::Secondary1);
        auto header_color = context.theme.scoped_color(ImGuiCol_Header, imgui::ThemeColors::Secondary1);
        {
            auto child_str = fmt::format("{}_child", title);
            imgui::ScopedChild child(
                child_str.data(),
                ImVec2(-std::numeric_limits<float>::min(), 0.0f),
                ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle
            );

            ImVec2 window_width = ImGui::GetContentRegionAvail();
            float delete_button_width = ImGui::CalcTextSize(ICON_FA_TRASH).x + ImGui::GetStyle().FramePadding.x * consts.icon_padding_scale;
            float option_button_width = ImGui::CalcTextSize(ICON_FA_GEARS).x + ImGui::GetStyle().FramePadding.x * consts.icon_padding_scale;

            {
                const imgui::ScopedTreeNodeEx tree_node(title.data(), tree_flags);
                ImGui::SameLine(window_width.x - delete_button_width - option_button_width);
                {
                    auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Primary1, 0.f);
                    if (removable)
                    {
                        if (ImGui::Button(ICON_FA_TRASH))
                        {
                            delete_component = true;
                            ImGui::OpenPopup(ICON_FA_TRASH " Delete Component");
                        }

                        imgui::confirm_and_execute(
                            delete_component,
                            ICON_FA_TRASH " Delete Component",
                            "Are you sure you want to delete this component?",
                            [&]()
                            {
                                entity.remove_component<T>();
                            },
                            context
                        );
                    }

                    ImGui::SameLine(window_width.x - option_button_width);
                    if (ImGui::Button(ICON_FA_GEARS))
                    {
                        ImGui::OpenPopup(ICON_FA_GEARS " Component Settings");
                    }
                    ImGui::PopStyleVar();

                    if (ImGui::BeginPopup(ICON_FA_GEARS " Component Settings"))
                    {
                        if (ImGui::MenuItem("Reset"))
                        {
                            entity.remove_component<T>();
                            entity.add_component<T>();
                        }
                        ImGui::EndPopup();
                    }
                }

                if (tree_node.is_open)
                {
                    const float avail_width = ImGui::GetContentRegionAvail().x;
                    const float margin_right = consts.margin_right; // pixels to leave empty on the right
                    imgui::ScopedChild dummy_child("dummy", ImVec2{avail_width - margin_right, 0.f}, ImGuiChildFlags_AutoResizeY);

                    if (entity.has_component<T>())
                    {
                        draw_func(context, entity);
                    }
                }
            }
        }
    }
};
} // portal
