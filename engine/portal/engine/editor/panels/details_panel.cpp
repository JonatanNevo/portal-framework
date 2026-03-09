//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "details_panel.h"

#include <glm/gtc/type_ptr.hpp>

#include "portal/engine/components/base.h"
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/light_components.h"
#include "portal/engine/components/mesh.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/editor/selection_system.h"
#include "portal/engine/imgui/utils.h"
#include "portal/engine/imgui/widgets/edit_vec3.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "portal/engine/scene/scene.h"
#include "portal/engine/scene/scene_context.h"
#include "portal/engine/systems/base_player_input_system.h"
#include "portal/third_party/font_awsome/IconsFontAwesome6.h"


namespace portal
{
namespace
{
    template <typename Primitive, typename Component, typename Func>
    bool is_inconsistent_primitive(ecs::Registry& registry, Entity scope, Func func)
    {
        const auto& selections = SelectionSystem::get_selections(scope);

        if (selections.size() < 2)
            return false;

        auto first_entity = registry.find_by_name(selections.front());
        if (!first_entity.has_value())
            return true;

        Primitive first_primitive = func(first_entity->get_component<Component>());

        for (auto& selection : selections | std::views::drop(1))
        {
            auto entity = registry.find_by_name(selection);

            if (!entity.has_value() || !entity->has_component<Component>())
                continue;

            const auto& other_value = func(entity->get_component<Component>());
            if (other_value != first_primitive)
                return true;
        }

        return false;
    }

    template <typename Component, typename Func>
    bool is_inconsistent_string(ecs::Registry& registry, const Entity scope, Func func)
    {
        return is_inconsistent_primitive<StringId, Component, Func>(registry, scope, func);
    }

    template <typename VectorType>
    imgui::VectorAxis get_inconsistent_vector_axis(VectorType& first, VectorType& second)
    {
        imgui::VectorAxis axis = imgui::VectorAxisBits::None;


        if (glm::epsilonNotEqual(second.x, first.x, FLT_EPSILON))
            axis |= imgui::VectorAxisBits::X;

        if (glm::epsilonNotEqual(second.y, first.y, FLT_EPSILON))
            axis |= imgui::VectorAxisBits::Y;

        if constexpr (std::is_same_v<VectorType, glm::vec3> || std::is_same_v<VectorType, glm::vec4>)
        {
            if (glm::epsilonNotEqual(second.z, first.z, FLT_EPSILON))
                axis |= imgui::VectorAxisBits::Z;
        }

        if constexpr (std::is_same_v<VectorType, glm::vec4>)
        {
            if (glm::epsilonNotEqual(second.w, first.w, FLT_EPSILON))
                axis |= imgui::VectorAxisBits::W;
        }

        return axis;
    }

    template <typename VectorType, typename Component, typename Func>
    imgui::VectorAxis get_inconsistent_vector_axis(ecs::Registry& registry, const Entity scope, Func&& func)
    {
        imgui::VectorAxis axis = imgui::VectorAxisBits::None;
        const auto& entities = SelectionSystem::get_selections(scope);

        if (entities.size() < 2)
            return axis;

        auto first_entity = registry.find_by_name(entities.front());
        if (!first_entity.has_value())
            return axis;

        VectorType first = func(first_entity->get_component<Component>());

        for (auto& id : entities | std::views::drop(1))
        {
            auto entity = registry.find_by_name(id);

            if (!entity.has_value() || !entity->has_component<Component>())
                continue;

            VectorType other_vector = func(entity->get_component<Component>());

            axis |= get_inconsistent_vector_axis(first, other_vector);
        }

        return axis;
    }
}

bool draw_vec3_control(
    EditorContext& context,
    const std::string_view label,
    glm::vec3& value,
    bool& manually_edited,
    const float reset_value = 0.f,
    const imgui::VectorAxis render_multi_selected_axes = imgui::VectorAxisBits::None,
    float speed = 1.f
)
{
    struct TransformVec3Consts
    {
        ImVec2 shift_cursor_label = {3.f, 0.f};
        ImVec2 shift_cursor_slider = {4.f, 0.f};
    };

    constexpr static TransformVec3Consts consts;

    bool modified = false;
    imgui::push_id();
    ImGui::TableSetColumnIndex(0);
    imgui::shift_cursor(consts.shift_cursor_label);

    ImGui::TextUnformatted(label.data());
    // imgui::underline(context.theme[imgui::ThemeColors::Primary2], false, 0.f, 0.f);

    ImGui::TableSetColumnIndex(1);
    imgui::shift_cursor(consts.shift_cursor_slider);

    modified = imgui::edit_vec3(
        context,
        label,
        ImVec2(ImGui::GetContentRegionAvail().x - 8.0f, ImGui::GetFrameHeightWithSpacing() + 8.0f),
        reset_value,
        manually_edited,
        value,
        render_multi_selected_axes,
        speed
    );

    imgui::pop_id();
    return modified;
}

template <>
struct ComponentEditorFunctions<TransformComponent>
{
    static constexpr bool removable = false;

    static void draw_details(
        EditorContext& context,
        Entity scene_entity,
        TransformComponent& first_component,
        std::span<const StringId> entities,
        bool is_multi_edit
    )
    {
        struct TransformDetailsConsts
        {
            ImVec2 item_spacing = {8.0f, 8.0f};
            ImVec2 frame_padding = {4.0f, 4.0f};
            float label_column_width = 100.0f;
            float shift_cursor_underline_y = 8.f; // 8.f
            float shift_cursor_end = 18.f;        // 18.f
        };

        constexpr static TransformDetailsConsts consts;

        imgui::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, consts.item_spacing);
        imgui::ScopedStyle padding(ImGuiStyleVar_FramePadding, consts.frame_padding);

        ImGui::BeginTable("TransformComponent", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoClip);
        ImGui::TableSetupColumn("label_column", 0, consts.label_column_width);
        ImGui::TableSetupColumn(
            "value_column",
            ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip,
            ImGui::GetContentRegionAvail().x - consts.label_column_width
        );

        bool translation_manually_edited = false;
        bool rotation_manually_edited = false;
        bool scale_manually_edited = false;

        if (is_multi_edit)
        {
            auto translation_axes = get_inconsistent_vector_axis<glm::vec3, TransformComponent>(
                context.ecs_registry,
                scene_entity,
                [](const TransformComponent& other) { return other.get_translation(); }
            );

            auto rotation_axes = get_inconsistent_vector_axis<glm::vec3, TransformComponent>(
                context.ecs_registry,
                scene_entity,
                [](const TransformComponent& other) { return other.get_rotation_euler(); }
            );

            auto scale_axes = get_inconsistent_vector_axis<glm::vec3, TransformComponent>(
                context.ecs_registry,
                scene_entity,
                [](const TransformComponent& other) { return other.get_scale(); }
            );

            auto translation = first_component.get_translation();
            auto rotation = glm::degrees(first_component.get_rotation_euler());
            auto scale = first_component.get_scale();

            auto old_translation = translation;
            auto old_rotation = rotation;
            auto old_scale = scale;

            ImGui::TableNextRow();
            bool changed = draw_vec3_control(context, "Translation", translation, translation_manually_edited, 0.0f, translation_axes, 0.1f);

            ImGui::TableNextRow();
            changed |= draw_vec3_control(context, "Rotation", rotation, rotation_manually_edited, 0.0f, rotation_axes);

            ImGui::TableNextRow();
            changed |= draw_vec3_control(context, "Scale", scale, scale_manually_edited, 1.0f, scale_axes);

            if (changed)
            {
                if (translation_manually_edited || rotation_manually_edited || scale_manually_edited)
                {
                    translation_axes = get_inconsistent_vector_axis<glm::vec3>(
                        translation,
                        old_translation
                    );

                    rotation_axes = get_inconsistent_vector_axis<glm::vec3>(
                        rotation,
                        old_rotation
                    );

                    scale_axes = get_inconsistent_vector_axis<glm::vec3>(
                        scale,
                        old_scale
                    );

                    for (auto& id : entities)
                    {
                        auto entity = context.ecs_registry.find_by_name(id);

                        entity->patch_component<TransformComponent>(
                            [&](TransformComponent& comp)
                            {
                                glm::vec3 new_translation = comp.get_translation();
                                if ((translation_axes & imgui::VectorAxisBits::X) != imgui::VectorAxisBits::None)
                                    new_translation.x = translation.x;
                                if ((translation_axes & imgui::VectorAxisBits::Y) != imgui::VectorAxisBits::None)
                                    new_translation.y = translation.y;
                                if ((translation_axes & imgui::VectorAxisBits::Z) != imgui::VectorAxisBits::None)
                                    new_translation.z = translation.z;
                                comp.set_translation(new_translation);

                                glm::vec3 new_rotation_euler = comp.get_rotation_euler();
                                if ((rotation_axes & imgui::VectorAxisBits::X) != imgui::VectorAxisBits::None)
                                    new_rotation_euler.x = glm::radians(rotation.x);
                                if ((rotation_axes & imgui::VectorAxisBits::Y) != imgui::VectorAxisBits::None)
                                    new_rotation_euler.y = glm::radians(rotation.y);
                                if ((rotation_axes & imgui::VectorAxisBits::Z) != imgui::VectorAxisBits::None)
                                    new_rotation_euler.z = glm::radians(rotation.z);
                                comp.set_rotation_euler(new_rotation_euler);

                                glm::vec3 new_scale = comp.get_scale();
                                if ((scale_axes & imgui::VectorAxisBits::X) != imgui::VectorAxisBits::None)
                                    new_scale.x = scale.x;
                                if ((scale_axes & imgui::VectorAxisBits::Y) != imgui::VectorAxisBits::None)
                                    new_scale.y = scale.y;
                                if ((scale_axes & imgui::VectorAxisBits::Z) != imgui::VectorAxisBits::None)
                                    new_scale.z = scale.z;
                                comp.set_scale(new_scale);
                            }
                        );
                    }
                }
                else
                {
                    for (auto& id : entities)
                    {
                        auto entity = context.ecs_registry.find_by_name(id);

                        entity->patch_component<TransformComponent>(
                            [&](TransformComponent& comp)
                            {
                                comp.set_translation(translation);
                                comp.set_rotation_euler(glm::radians(rotation));
                                comp.set_scale(scale);
                            }
                        );
                    }
                }
            }
        }
        else
        {
            auto entity = context.ecs_registry.find_by_name(entities[0]);

            glm::vec3 translation = first_component.get_translation();
            glm::vec3 rotation = glm::degrees(first_component.get_rotation_euler());
            glm::vec3 scale = first_component.get_scale();

            ImGui::TableNextRow();
            if (draw_vec3_control(context, "Translation", translation, translation_manually_edited, 0.0f))
            {
                entity->patch_component<TransformComponent>(
                    [&](TransformComponent& comp)
                    {
                        comp.set_translation(translation);
                    }
                );
            }

            ImGui::TableNextRow();
            if (draw_vec3_control(context, "Rotation", rotation, rotation_manually_edited, 0.0f))
            {
                entity->patch_component<TransformComponent>(
                    [&](TransformComponent& comp)
                    {
                        comp.set_rotation_euler(glm::radians(rotation));
                    }
                );
            }

            ImGui::TableNextRow();
            if (draw_vec3_control(context, "Scale", scale, scale_manually_edited, 1.0f))
            {
                entity->patch_component<TransformComponent>(
                    [&](TransformComponent& comp)
                    {
                        comp.set_scale(scale);
                    }
                );
            }
        }

        ImGui::EndTable();
    }
};

template <>
struct ComponentEditorFunctions<StaticMeshComponent>
{
    static constexpr bool removable = false;

    static void draw_details(
        EditorContext& context,
        Entity scene_entity,
        StaticMeshComponent& first_component,
        std::span<const StringId> entities,
        bool is_multi_edit
    )
    {
        // struct StaticMeshDetailsConsts
        // {
        // };
        //
        // constexpr static StaticMeshDetailsConsts consts;

        imgui::begin_property_grid();
        ImGui::PushItemFlag(
            ImGuiItemFlags_MixedValue,
            is_multi_edit && is_inconsistent_primitive<bool, StaticMeshComponent>(
                context.ecs_registry,
                scene_entity,
                [](const StaticMeshComponent& other) { return other.visible; }
            )
        );
        if (imgui::property("Visible", first_component.visible))
        {
            for (auto& id : entities)
            {
                auto entity = context.ecs_registry.find_by_name(id);
                auto& mesh = entity->get_component<StaticMeshComponent>();
                mesh.visible = first_component.visible;
            }
        }
        ImGui::PopItemFlag();

        auto mesh_ref = first_component.mesh;

        ImGui::PushItemFlag(
            ImGuiItemFlags_MixedValue,
            is_multi_edit && is_inconsistent_primitive<StringId, StaticMeshComponent>(
                context.ecs_registry,
                scene_entity,
                [](const StaticMeshComponent& other) { return other.mesh->get_id(); }
            )
        );

        auto res = imgui::property_resource_reference_with_conversion<MeshGeometry, MeshGeometry>(
            context,
            "Static Mesh",
            mesh_ref,
            [](ResourceReference<MeshGeometry>) {}
        );
        if (res.has_value() && res.value())
        {
            for (auto& id : entities)
            {
                // TODO: add snapshot for undo
                auto entity = context.ecs_registry.find_by_name(id);
                entity->patch_component<StaticMeshComponent>([&mesh_ref](StaticMeshComponent& comp) { comp.mesh = mesh_ref; });
            }
        }

        ImGui::PopItemFlag();
        imgui::end_property_grid();
    }
};

//
// template <>
// struct ComponentEditorFunctions<CameraComponent>
// {
//     static constexpr bool removable = false;
//
//     static void draw_details(
//         EditorContext& context,
//         Entity scene_entity,
//         CameraComponent& first_component,
//         std::span<const StringId> entities,
//         bool is_multi_edit
//     )
//     {
//     }
// };
//
// template <>
// struct ComponentEditorFunctions<DirectionalLightComponent>
// {
//     static constexpr bool removable = false;
//
//     static void draw_details(
//         EditorContext& context,
//         Entity scene_entity,
//         DirectionalLightComponent& first_component,
//         std::span<const StringId> entities,
//         bool is_multi_edit
//     )
//     {
//     }
// };
//
// template <>
// struct ComponentEditorFunctions<PointLightComponent>
// {
//     static constexpr bool removable = false;
//
//     static void draw_details(
//         EditorContext& context,
//         Entity scene_entity,
//         PointLightComponent& first_component,
//         std::span<const StringId> entities,
//         bool is_multi_edit
//     )
//     {
//     }
// };
//
// template <>
// struct ComponentEditorFunctions<SpotlightComponent>
// {
//     static constexpr bool removable = false;
//
//     static void draw_details(
//         EditorContext& context,
//         Entity scene_entity,
//         SpotlightComponent& first_component,
//         std::span<const StringId> entities,
//         bool is_multi_edit
//     )
//     {
//     }
// };
//
// template <>
// struct ComponentEditorFunctions<SkylightComponent>
// {
//     static constexpr bool removable = false;
//
//     static void draw_details(
//         EditorContext& context,
//         Entity scene_entity,
//         SkylightComponent& first_component,
//         std::span<const StringId> entities,
//         bool is_multi_edit
//     )
//     {
//     }
// };


void DetailsPanel::on_gui_render(EditorContext& context, FrameContext& frame, bool& is_open)
{
    auto scene = std::any_cast<SceneContext>(&frame.scene_context)->active_scene;\
    bool show_changes = static_cast<bool>(scene.get_dirty() & ResourceDirtyBits::DataChange);

    std::string window_title = show_changes
                                   ? ICON_FA_SLIDERS " Details*###Details"
                                   : ICON_FA_SLIDERS " Details###Details";
    imgui::ScopedWindow details_window(window_title.c_str(), &is_open);

    if (!SelectionSystem::has_selection(scene->get_scene_entity()))
        return;

    auto selections = SelectionSystem::get_selections(scene->get_scene_entity());
    std::vector<Entity> entities;
    for (auto& selection : selections)
    {
        auto entity = context.ecs_registry.find_by_name(selection);
        if (entity.has_value())
            entities.push_back(entity.value());
    }

    draw_components(context, scene->get_scene_entity(), entities);
}

void DetailsPanel::draw_components(EditorContext& context, Entity scene_entity, std::span<Entity> entities)
{
    struct DrawComponentConsts
    {
        ImVec2 first_cursor_shift = ImVec2(4.f, 4.f);
        float icon_offset = 6.f;
        float icon_x_offset = 4.f;
        ImVec2 add_cursor_shift = ImVec2(-5.f, 0.f);
        float padding = 4.f;
        ImVec2 add_image_shift = ImVec2{-5.f, -1.f};
        float add_component_width = 250.f;
    };

    constexpr static DrawComponentConsts consts;

    if (entities.empty())
        return;

    auto frame_background = context.theme.scoped_color(ImGuiCol_FrameBg, imgui::ThemeColors::Primary2);

    ImGui::AlignTextToFramePadding();
    auto region_available = ImGui::GetContentRegionAvail();

    imgui::shift_cursor(consts.first_cursor_shift);

    const bool multiselect = entities.size() > 1;
    auto first_entity = entities.front();

    // Name
    {
        imgui::shift_cursor(consts.icon_x_offset, consts.icon_offset);

        auto edit_icon = context.icons.get_texture(EditorIcon::Edit);
        ImGui::Image(
            static_cast<VkDescriptorSet>(context.icons.get_descriptor(EditorIcon::Edit)),
            ImVec2(14.f, 14.f),
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f),
            context.theme[imgui::ThemeColors::Text],
            ImVec4(0, 0, 0, 0)
        );

        ImGui::SameLine();
        imgui::shift_cursor(0, -consts.icon_offset);

        const bool inconsistent_name = is_inconsistent_string<NameComponent>(
            context.ecs_registry,
            scene_entity,
            [](const NameComponent& comp) { return comp.name; }
        );

        auto name = (multiselect && inconsistent_name) ? "---" : first_entity.get_component<NameComponent>().name.string;

        std::array<char, 256> buffer{'\0'};
        std::memcpy(buffer.data(), name.data(), std::min(name.size(), buffer.size()));

        ImGui::PushItemWidth(region_available.x * 0.5f);
        imgui::ScopedStyle disable_frame_border(ImGuiStyleVar_FrameBorderSize, 0.f);
        imgui::ScopedFont bold_font(STRING_ID("Bold"));
        auto frame_color = context.theme.scoped_color(ImGuiCol_FrameBg, imgui::ThemeColors::Background1);

        // if (entities.size() > 1)
        ImGui::BeginDisabled();
        // Because entity names are unique for now and my selection system is based on entity names, renaming is not yet supported...

        if (ImGui::InputText("##Name", buffer.data(), buffer.size()))
        {
            first_entity.patch_component<NameComponent>(
                [buffer](NameComponent& comp)
                {
                    if (buffer[0] == '\0')
                    {
                        comp.name = STRING_ID("Unnamed Entity");
                    }
                    else
                    {
                        comp.name = STRING_ID(std::string_view(buffer.data(), std::strlen(buffer.data())));
                    }
                }
            );
        }

        // if (entities.size() > 1)
        ImGui::EndDisabled();

        imgui::draw_item_activity_outline(imgui::OutlineFlags_NoOutlineInactive);

        ImGui::PopItemWidth();
    }

    ImGui::SameLine();
    imgui::shift_cursor(consts.add_cursor_shift);

    float line_height = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.f;

    auto add_text_size = ImGui::CalcTextSize(" ADD        ");
    add_text_size.x += GImGui->Style.FramePadding.y * 2.f;

    {
        auto button_color = context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Primary1, 0.8f);
        auto button_hovered = context.theme.scoped_color(ImGuiCol_ButtonHovered, imgui::ThemeColors::Primary1, 1.f);
        auto button_active = context.theme.scoped_color(ImGuiCol_ButtonActive, imgui::ThemeColors::Primary1, 0.6f);

        ImGui::SameLine(region_available.x - (add_text_size.x + GImGui->Style.FramePadding.x));
        if (ImGui::Button(" ADD        ", ImVec2(add_text_size.x + 4.f, line_height + 2.f)))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        const float icon_width = ImGui::GetFrameHeight() - consts.padding * 2.f;
        const float icon_height = icon_width;
        ImVec2 icon_position = ImGui::GetItemRectMax();
        icon_position.x -= icon_width + consts.padding;
        icon_position.y -= icon_height + consts.padding;

        ImGui::SetCursorScreenPos(icon_position);
        imgui::shift_cursor(consts.add_image_shift);

        ImGui::Image(
            static_cast<VkDescriptorSet>(context.icons.get_descriptor(EditorIcon::Add)),
            ImVec2(icon_width, icon_height),
            ImVec2(0.f, 0.f),
            ImVec2(1.f, 1.f),
            context.theme[imgui::ThemeColors::Text],
            ImVec4(0, 0, 0, 0)
        );
    }

    float add_comp_panel_start_y = ImGui::GetCursorScreenPos().y;

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    {
        imgui::ScopedFont bont(STRING_ID("Bold"));
        imgui::ScopedStyle disable_item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
        imgui::ScopedStyle window_padding(ImGuiStyleVar_WindowPadding, ImVec2{5.f, 10.f});
        imgui::ScopedStyle popup_rounding(ImGuiStyleVar_PopupRounding, 4.f);
        imgui::ScopedStyle disable_cell_padding(ImGuiStyleVar_CellPadding, ImVec2{0, 0});

        auto window_pos = ImGui::GetWindowPos();
        const float max_height = ImGui::GetContentRegionAvail().y - 60.f;

        ImGui::SetNextWindowPos({window_pos.x + consts.add_component_width / 1.3f, add_comp_panel_start_y});
        ImGui::SetNextWindowSizeConstraints({50.f, 50.f}, {consts.add_component_width, max_height});
        if (ImGui::BeginPopup("AddComponentPopup", ImGuiWindowFlags_NoDocking))
        {
            ImGui::TextUnformatted("Not Implemented!");

            ImGui::EndPopup();
        }

        // TODO: automatically call for all registered components
        draw_component<TransformComponent>(context, "Transform", scene_entity);
        draw_component<StaticMeshComponent>(context, "Static Mesh", scene_entity);
    }

    // draw_component<TransformComponent>(
    //     context,
    //     ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform",
    //     selected_entity,
    //     [&scene](EditorContext& editor_context, const Entity& entity)
    //     {
    //         show_transform_controls(scene, editor_context, entity);
    //     }
    // );
    //
    // draw_component<CameraComponent>(
    //     context,
    //     ICON_FA_VIDEO " Camera",
    //     selected_entity,
    //     [&scene](const EditorContext& editor_context, Entity& entity)
    //     {
    //         auto& camera = entity.get_component<CameraComponent>();
    //         bool changed = false;
    //
    //         imgui::ScopedStyle frame_padding(ImGuiStyleVar_FramePadding, ImVec2(4, 1));
    //         {
    //             auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
    //             ImGui::AlignTextToFramePadding();
    //             ImGui::Text("Main Camera:");
    //         }
    //         ImGui::SameLine();
    //         {
    //             auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text);
    //             auto frame_background = editor_context.theme.scoped_color(ImGuiCol_FrameBg, imgui::ThemeColors::Primary1);
    //
    //             ImGui::BeginDisabled();
    //             auto is_main = entity.has_component<MainCameraTag>();
    //             ImGui::Checkbox("##MainCameraCheckbox", &is_main);
    //             ImGui::EndDisabled();
    //         }
    //
    //         {
    //             auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
    //             ImGui::AlignTextToFramePadding();
    //             ImGui::Text("Vertical FOV:");
    //         }
    //         ImGui::SameLine();
    //         {
    //             ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    //             changed |= ImGui::DragFloat("##VerticalFOV", &camera.vertical_fov, 0.1f, 0.0f, 180.0f);
    //         }
    //
    //         {
    //             auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
    //             ImGui::AlignTextToFramePadding();
    //             ImGui::Text("Far Clip:");
    //         }
    //         ImGui::SameLine();
    //         {
    //             ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    //             changed |= ImGui::DragFloat("##FarClip", &camera.near_clip, 0.1f, 0.0f, std::numeric_limits<float>::max());
    //         }
    //
    //         {
    //             auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
    //             ImGui::AlignTextToFramePadding();
    //             ImGui::Text("Near Clip:");
    //         }
    //         ImGui::SameLine();
    //         {
    //             ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    //             changed |= ImGui::DragFloat("##NearClip", &camera.far_clip, 0.1f, 0.0f, std::numeric_limits<float>::max());
    //         }
    //
    //         if (entity.has_component<BaseCameraController>())
    //         {
    //             auto& controller = entity.get_component<BaseCameraController>();
    //             {
    //                 auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
    //                 ImGui::AlignTextToFramePadding();
    //                 ImGui::Text("Camera Speed:");
    //             }
    //             ImGui::SameLine();
    //             {
    //                 ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    //                 ImGui::DragFloat("##Speed", &controller.speed, 0.01f, 0.0f, std::numeric_limits<float>::max());
    //             }
    //         }
    //
    //         if (changed)
    //         {
    //             camera.calculate_projection();
    //             scene.set_dirty(ResourceDirtyBits::DataChange);
    //         }
    //     }
    // );
    //
    // draw_component<StaticMeshComponent>(
    //     context,
    //     ICON_FA_CUBE " Static Mesh",
    //     selected_entity,
    //     [](EditorContext& editor_context, const Entity& entity)
    //     {
    //         draw_material_controls(editor_context, entity);
    //     }
    // );
}
} // portal
