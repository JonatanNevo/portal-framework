//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "details_panel.h"

#include "portal/engine/components/base.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/editor/selection_manager.h"
#include "portal/engine/scene/scene.h"
#include "portal/third_party/font_awsome/IconsFontAwesome6.h"


namespace portal
{
struct TransformVec3Consts
{
    float column_width = 100.f;
    float frame_padding_scale = 2.f;
    float columns_width_offset = 30.f;
};

template <typename Func>
void transform_vec3_slider(
    const EditorContext& context,
    const char* label,
    glm::vec3 vector,
    const Func& on_changed,
    float reset_val = 0.0f
)
{
    static TransformVec3Consts consts{};
    imgui::draw_consts_controls("Vec3 Consts Controls", consts);

    imgui::ScopedID scoped_id(label);

    ImGui::AlignTextToFramePadding();
    ImGui::Columns(2);
    // width of the 1st column (labels)
    ImGui::SetColumnWidth(0, consts.column_width);
    float line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * consts.frame_padding_scale;
    auto button_size = ImVec2(line_height * 0.7f, line_height);

    {
        auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text2);
        ImGui::Text("%s", label);
    }


    ImGui::NextColumn();
    ImGui::PushMultiItemsWidths(3, ImGui::GetContentRegionAvail().x - consts.columns_width_offset);
    {
        imgui::ScopedStyle disable_item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
        auto button_active_color = context.theme.scoped_color(ImGuiCol_ButtonActive, imgui::ThemeColors::Secondary2);
        {
            auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::X);
            auto button_hovered_color = context.theme.scoped_color(ImGuiCol_ButtonHovered, imgui::ThemeColors::X);
            auto x_id = fmt::format("##X_{}", label);
            imgui::ScopedID scoped_x_id(x_id.c_str());

            ImGui::SetNextItemWidth(line_height);
            {
                auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Secondary2);
                imgui::ScopedFont bold_font(STRING_ID("Bold"));
                if (ImGui::Button("X", button_size))
                {
                    on_changed({reset_val, vector.y, vector.z});
                }
            }
            ImGui::SameLine();
            if (ImGui::DragFloat("##X", &vector.x, 0.01f))
            {
                on_changed(vector);
            }
            ImGui::SameLine();
            ImGui::PopItemWidth();
        }

        {
            auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Y);
            auto button_hovered_color = context.theme.scoped_color(ImGuiCol_ButtonHovered, imgui::ThemeColors::Y);
            auto y_id = fmt::format("##Y_{}", label);
            imgui::ScopedID scoped_y_id(y_id.c_str());

            ImGui::SetNextItemWidth(line_height);
            {
                imgui::ScopedFont bold_font(STRING_ID("Bold"));
                auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Secondary2);
                if (ImGui::Button("Y", button_size))
                {
                    on_changed({vector.y, reset_val, vector.z});
                }
            }
            ImGui::SameLine();
            if (ImGui::DragFloat("##Y", &vector.y, 0.01f))
            {
                on_changed(vector);
            }
            ImGui::SameLine();
            ImGui::PopItemWidth();
        }


        {
            auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Z);
            auto button_hovered_color = context.theme.scoped_color(ImGuiCol_ButtonHovered, imgui::ThemeColors::Z);
            auto z_id = fmt::format("##Z_{}", label);
            imgui::ScopedID scoped_z_id(z_id.c_str());

            ImGui::SetNextItemWidth(line_height);
            {
                imgui::ScopedFont bold_font(STRING_ID("Bold"));
                auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Secondary2);
                if (ImGui::Button("Z", button_size))
                {
                    on_changed({vector.x, vector.y, reset_val});
                }
            }
            ImGui::SameLine();
            if (ImGui::DragFloat("##Z", &vector.z, 0.01f))
            {
                on_changed(vector);
            }
            ImGui::SameLine();
            ImGui::PopItemWidth();
        }
    }
    ImGui::Columns(1);
}

void show_transform_controls(const EditorContext& context, Entity entity)
{
    auto& transform = entity.get_component<TransformComponent>();

    imgui::ScopedStyle item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2{5, 2});
    transform_vec3_slider(
        context,
        "Position",
        transform.get_translation(),
        [&](glm::vec3 vector)
        {
            entity.patch_component<TransformComponent>(
                [&](TransformComponent& comp)
                {
                    comp.set_translation(vector);
                }
            );
        }
    );

    transform_vec3_slider(
        context,
        "Rotation",
        transform.get_rotation_euler(),
        [&](glm::vec3 vector)
        {
            entity.patch_component<TransformComponent>(
                [&](TransformComponent& comp)
                {
                    comp.set_rotation_euler(vector);
                }
            );
        }
    );

    transform_vec3_slider(
        context,
        "Scale",
        transform.get_scale(),
        [&](glm::vec3 vector)
        {
            entity.patch_component<TransformComponent>(
                [&](TransformComponent& comp)
                {
                    comp.set_scale(vector);
                }
            );
        },
        1.0f
    );
}


void DetailsPanel::on_gui_render(EditorContext& context, FrameContext& frame)
{
    ImGui::SetNextWindowSizeConstraints({350, 50}, {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()});
    imgui::ScopedWindow details_window(ICON_FA_SLIDERS " Details");

    if (!SelectionSystem::has_selection(frame.active_scene->get_scene_entity()))
        return;
    auto selected_entity = SelectionSystem::get_selected_entity(frame.active_scene->get_scene_entity());

    auto frame_background = context.theme.scoped_color(ImGuiCol_FrameBg, imgui::ThemeColors::Primary2);

    if (selected_entity.has_component<NameComponent>())
    {
        auto& [name, icon] = selected_entity.get_component<NameComponent>();
        {
            auto title_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text2);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Name:", icon);
        }
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        std::array<char, 256> name_buffer{};
        std::ranges::copy(name.string, name_buffer.begin());

        if (ImGui::InputTextWithHint("##NameComp", "Entity Name", name_buffer.data(), name_buffer.size()))
        {
            selected_entity.patch_component<NameComponent>(
                [&name_buffer](NameComponent& comp) { comp.name = STRING_ID(std::string(name_buffer.data())); }
            );
        }
    }

    draw_component<TransformComponent>(
        context,
        ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform",
        selected_entity,
        [](const EditorContext& editor_context, const Entity& entity)
        {
            show_transform_controls(editor_context, entity);
        }
    );

    draw_component<CameraComponent>(
        context,
        ICON_FA_VIDEO " Camera",
        selected_entity,
        [](const EditorContext&, const Entity&)
        {

        }
    );
}
} // portal
