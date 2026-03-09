//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "edit_vec3.h"

#include "portal/engine/imgui/utils.h"

namespace portal::imgui
{
struct EditVec3Consts
{
    ImVec2 spacing = ImVec2{8.f, 0.f};
    ImVec2 padding = ImVec2{0.f, 2.f};
    float frame_padding = 2.f;
    float outline_spacing = 1.f;
    float size_addition_x = 2.f;
    float input_item_width_scale = 3.f;
    float button_rounding = 1.f;
    float input_padding_y = 2.5f;
};

bool edit_vec3(
    EditorContext& context,
    std::string_view label,
    ImVec2 size,
    float reset_value,
    bool& manually_edited,
    glm::vec3& value,
    VectorAxis render_multi_select_axes,
    float speed,
    glm::vec3 v_min,
    glm::vec3 v_max,
    const char* format,
    ImGuiSliderFlags flags
)
{
    static constexpr EditVec3Consts consts;

    ImGui::BeginVertical((std::string(label) + "vertical").data());
    bool changed = false;

    auto on_activate = [&]() mutable
    {
        context.snapshot_manager.prepare_snapshot(STRING_ID(fmt::format("Update {}", label)));
    };

    auto on_deactivate = [&]() mutable
    {
        context.snapshot_manager.commit_snapshot();
    };

    ScopedStyle item_spacing(ImGuiStyleVar_ItemSpacing, consts.spacing);
    ScopedStyle padding(ImGuiStyleVar_WindowPadding, consts.padding);
    const float line_height = GImGui->FontSize + consts.frame_padding * 2.f;

    const auto button_size = ImVec2(line_height + consts.size_addition_x, line_height);
    const float input_item_width = size.x / consts.input_item_width_scale - button_size.x;

    shift_cursor(0.f, consts.frame_padding);

    auto draw_control = [&](
        const std::string& value_label,
        float& single_value,
        const ImVec4& color_normal,
        const ImVec4 color_hovered,
        const ImVec4 color_active,
        const bool render_multi_select,
        const float change_speed,
        const float value_min,
        const float value_max,
        const char* drag_format,
        const ImGuiSliderFlags slider_flags
    )
    {
        ScopedStyle button_frame(ImGuiStyleVar_FramePadding, ImVec2{consts.frame_padding, 0.f});
        ScopedStyle button_rounding(ImGuiStyleVar_FrameRounding, consts.button_rounding);
        ScopedStyle disable_item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        ScopedColor button_color(ImGuiCol_Button, color_normal);
        ScopedColor button_hovered_color(ImGuiCol_ButtonHovered, color_hovered);
        ScopedColor button_active_color(ImGuiCol_ButtonActive, color_active);

        const auto id = fmt::format("##{}", label);
        ScopedID scoped_id(id.c_str());

        ImGui::SetNextItemWidth(line_height);

        shift_cursor(0.f, consts.frame_padding / 2.f);
        auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text);
        ScopedFont bold_font(STRING_ID("Bold"));
        const auto button_clicked = ImGui::Button(value_label.c_str(), button_size);
        if (ImGui::IsItemActivated())
        {
            on_activate();
        }
        if (button_clicked)
        {
            //   on_changed({reset_val, vector.y, vector.z});
            single_value = reset_value;
            changed = true;
        }
        if (ImGui::IsItemDeactivated())
        {
            on_deactivate();
        }
        shift_cursor(0.f, -consts.frame_padding / 2.f);

        ImGui::SameLine(0.f, consts.outline_spacing);
        ImGui::SetNextItemWidth(input_item_width);

        shift_cursor(0.f, consts.input_padding_y);

        ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, render_multi_select);
        const bool was_temp_input_active = ImGui::TempInputIsActive(ImGui::GetID(("##" + value_label).c_str()));
        const auto drag_changed = ImGui::DragFloat(
            ("##" + value_label).c_str(),
            &single_value,
            change_speed,
            value_min,
            value_max,
            drag_format,
            slider_flags
        );
        if (ImGui::IsItemActivated())
            on_activate();
        if (drag_changed)
            changed |= drag_changed;
        if (ImGui::IsItemDeactivatedAfterEdit())
            on_deactivate();

        if (changed && ImGui::IsKeyDown(ImGuiKey_Tab))
            manually_edited = true;

        if (ImGui::TempInputIsActive(ImGui::GetID(("##" + value_label).c_str())))
            changed = false;

        ImGui::PopItemFlag();

        if (was_temp_input_active)
            manually_edited |= ImGui::IsItemDeactivatedAfterEdit();
    };

    draw_control(
        "X",
        value.x,
        context.theme.get_color(ThemeColors::X, 0.8f),
        context.theme.get_color(ThemeColors::X),
        context.theme.get_color(ThemeColors::X, 0.8f),
        (render_multi_select_axes & VectorAxisBits::X) == VectorAxisBits::X,
        speed,
        v_min.x,
        v_max.x,
        format,
        flags
    );

    ImGui::SameLine(0.0f, consts.outline_spacing);
    draw_control(
        "Y",
        value.y,
        context.theme.get_color(ThemeColors::Y, 0.8f),
        context.theme.get_color(ThemeColors::Y),
        context.theme.get_color(ThemeColors::Y, 0.8f),
        (render_multi_select_axes & VectorAxisBits::Y) == VectorAxisBits::Y,
        speed,
        v_min.y,
        v_max.y,
        format,
        flags
    );

    ImGui::SameLine(0.0f, consts.outline_spacing);
    draw_control(
        "Z",
        value.z,
        context.theme.get_color(ThemeColors::Z, 0.8f),
        context.theme.get_color(ThemeColors::Z),
        context.theme.get_color(ThemeColors::Z, 0.8f),
        (render_multi_select_axes & VectorAxisBits::Z) == VectorAxisBits::Z,
        speed,
        v_min.z,
        v_max.z,
        format,
        flags
    );

    ImGui::EndVertical();

    return changed || manually_edited;
}
}


// struct TransformVec3Consts
// {
//     float column_width = 70.f;
//     float frame_padding_scale = 2.f;
//     float columns_width_offset = 30.f;
// };

// template <typename Func>
// void transform_vec3_slider(
//     EditorContext& context,
//     const char* label,
//     glm::vec3 vector,
//     const Func& on_changed,
//     float reset_val = 0.0f
// )
// {
//     constexpr TransformVec3Consts consts{};
//     imgui::ScopedID scoped_id(label);
//
//     auto on_activate = [&]() mutable
//     {
//         context.snapshot_manager.prepare_snapshot(STRING_ID(fmt::format("Update {}", label)));
//     };
//
//     auto on_deactivate = [&]() mutable
//     {
//         context.snapshot_manager.commit_snapshot();
//     };
//
//     ImGui::AlignTextToFramePadding();
//     ImGui::Columns(2);
//     // width of the 1st column (labels)
//     ImGui::SetColumnWidth(0, consts.column_width);
//     float line_height = GImGui->FontSize + GImGui->Style.FramePadding.y * consts.frame_padding_scale;
//     auto button_size = ImVec2(line_height * 0.7f, line_height);
//
//     {
//         auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
//         ImGui::Text("%s", label);
//     }
//
//
//     ImGui::NextColumn();
//     ImGui::PushMultiItemsWidths(3, ImGui::GetContentRegionAvail().x - consts.columns_width_offset);
//     {
//         imgui::ScopedStyle disable_item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
//         auto button_active_color = context.theme.scoped_color(ImGuiCol_ButtonActive, imgui::ThemeColors::Secondary2);
//         {
//             auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::X);
//             auto button_hovered_color = context.theme.scoped_color(ImGuiCol_ButtonHovered, imgui::ThemeColors::X);
//             auto x_id = fmt::format("##X_{}", label);
//             imgui::ScopedID scoped_x_id(x_id.c_str());
//
//             ImGui::SetNextItemWidth(line_height);
//             {
//                 auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Secondary2);
//                 imgui::ScopedFont bold_font(STRING_ID("Bold"));
//                 auto button_clicked = ImGui::Button("X", button_size);
//                 if (ImGui::IsItemActivated())
//                     on_activate();
//                 if (button_clicked)
//                     on_changed({reset_val, vector.y, vector.z});
//                 if (ImGui::IsItemDeactivated())
//                     on_deactivate();
//             }
//             ImGui::SameLine();
//
//             auto drag_changed = ImGui::DragFloat("##X", &vector.x, 0.01f);
//             if (ImGui::IsItemActivated())
//                 on_activate();
//             if (drag_changed)
//                 on_changed(vector);
//             if (ImGui::IsItemDeactivatedAfterEdit())
//                 on_deactivate();
//
//             ImGui::SameLine();
//             ImGui::PopItemWidth();
//         }
//
//         {
//             auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Y);
//             auto button_hovered_color = context.theme.scoped_color(ImGuiCol_ButtonHovered, imgui::ThemeColors::Y);
//             auto y_id = fmt::format("##Y_{}", label);
//             imgui::ScopedID scoped_y_id(y_id.c_str());
//
//             ImGui::SetNextItemWidth(line_height);
//             {
//                 imgui::ScopedFont bold_font(STRING_ID("Bold"));
//                 auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Secondary2);
//                 auto button_clicked = ImGui::Button("Y", button_size);
//                 if (ImGui::IsItemActivated())
//                     on_activate();
//                 if (button_clicked)
//                     on_changed({reset_val, vector.y, vector.z});
//                 if (ImGui::IsItemDeactivated())
//                     on_deactivate();
//             }
//             ImGui::SameLine();
//
//             auto drag_changed = ImGui::DragFloat("##Y", &vector.y, 0.01f);
//             if (ImGui::IsItemActivated())
//                 on_activate();
//             if (drag_changed)
//                 on_changed(vector);
//             if (ImGui::IsItemDeactivatedAfterEdit())
//                 on_deactivate();
//
//             ImGui::SameLine();
//             ImGui::PopItemWidth();
//         }
//
//
//         {
//             auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Z);
//             auto button_hovered_color = context.theme.scoped_color(ImGuiCol_ButtonHovered, imgui::ThemeColors::Z);
//             auto z_id = fmt::format("##Z_{}", label);
//             imgui::ScopedID scoped_z_id(z_id.c_str());
//
//             ImGui::SetNextItemWidth(line_height);
//             {
//                 imgui::ScopedFont bold_font(STRING_ID("Bold"));
//                 auto text_color = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Secondary2);
//                 auto button_clicked = ImGui::Button("Z", button_size);
//                 if (ImGui::IsItemActivated())
//                     on_activate();
//                 if (button_clicked)
//                     on_changed({vector.x, vector.y, reset_val});
//                 if (ImGui::IsItemDeactivated())
//                     on_deactivate();
//             }
//             ImGui::SameLine();
//
//             auto drag_changed = ImGui::DragFloat("##Z", &vector.z, 0.01f);
//             if (ImGui::IsItemActivated())
//                 on_activate();
//             if (drag_changed)
//                 on_changed(vector);
//             if (ImGui::IsItemDeactivatedAfterEdit())
//                 on_deactivate();
//
//             ImGui::SameLine();
//             ImGui::PopItemWidth();
//         }
//     }
//     ImGui::Columns(1);
// }
