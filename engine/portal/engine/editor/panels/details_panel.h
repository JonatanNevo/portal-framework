//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "panel.h"
#include <imgui.h>

#include "components/component_editor.h"
#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/engine/imgui/tree_node_with_icon.h"
#include "portal/engine/ecs/entity.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/editor/selection_system.h"
#include "portal/engine/imgui/utils.h"
#include "portal/third_party/font_awsome/IconsFontAwesome6.h"

namespace portal
{

class DetailsPanel final : public Panel
{
public:
    void on_gui_render(EditorContext& context, FrameContext& frame, bool& is_open) override;

private:
    void draw_components(EditorContext& context, Entity scene_entity, std::span<Entity> entities);

private:
    template <typename T>
    static void draw_component(
        EditorContext& context,
        const std::string_view name,
        Entity& scene_entity,
        vk::DescriptorSet icon = nullptr
    )
    {
        struct DrawComponentConsts
        {
            float item_padding = 4.f;
            float same_line_options = -7.f;
            ImVec2 options_icon_size = {20.f, 20.f};
            float shift_line_scale_y_options = 3.f;
        };

        constexpr DrawComponentConsts consts{};

        constexpr ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowOverlap
            | ImGuiTreeNodeFlags_FramePadding
            | ImGuiTreeNodeFlags_DefaultOpen;

        if constexpr (!HasDetailsFunction<T>)
            return;

        bool should_draw = true;

        auto& entities = SelectionSystem::get_selections(scene_entity);
        for (auto& id : entities)
        {
            auto entity = context.ecs_registry.find_by_name(id);
            if (!entity.has_value())
            {
                should_draw = false;
                break;
            }

            if (!entity->template has_component<T>())
            {
                should_draw = false;
                break;
            }
        }

        if (!should_draw || entities.empty())
            return;

        if (icon == nullptr)
            icon = context.icons.get_descriptor(EditorIcon::Resource);

        //  This fixes an issue where the first "+" button would display the "Remove" buttons for ALL components on an Entity.
        //  This is due to ImGui::TreeNodeEx only pushing the id for it's children if it's actually open
        ImGui::PushID(reinterpret_cast<void*>(typeid(T).hash_code()));
        ImVec2 content_region_available = ImGui::GetContentRegionAvail();

        bool open = tree_node_with_icon(icon, icon, name.data(), tree_flags);
        bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
        float line_height = ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y;

        bool reset_values = false;
        bool remove_component = false;

        ImGui::SameLine(content_region_available.x - consts.options_icon_size.x - consts.same_line_options);
        // imgui::shift_cursor(0.f, line_height / consts.shift_line_scale_y_options);
        imgui::draw_button_image(
            context.icons.get_descriptor(EditorIcon::Settings),
            context.theme.get_color(imgui::ThemeColors::Text, 0.8f),
            context.theme.get_color(imgui::ThemeColors::Text, 1.f),
            context.theme.get_color(imgui::ThemeColors::Text, 0.6f),
            consts.options_icon_size
        );

        if (ImGui::InvisibleButton("##options", ImVec2(line_height, line_height)) || right_clicked)
        {
            ImGui::OpenPopup("ComponentOptions");
        }

        if (ImGui::BeginPopup("ComponentOptions"))
        {
            {
                imgui::ScopedStyle item_padding(ImGuiStyleVar_FramePadding, ImVec2{consts.item_padding, 0.f});

                auto entity = context.ecs_registry.find_by_name(entities.front());

                if constexpr (HasOptionsFunction<T>)
                {
                    ComponentEditorFunctions<T>::draw_options(context, entity);
                }

                if (ImGui::MenuItem("Reset"))
                    reset_values = true;

                if constexpr (ComponentEditorFunctions<T>::removable)
                {
                    if (ImGui::MenuItem("Remove"))
                        remove_component = true;
                }
            }

            ImGui::EndPopup();
        }

        if (open)
        {
            auto entity = context.ecs_registry.find_by_name(entities.front());
            auto& first_comp = entity->template get_component<T>();

            const bool is_multi_edit = entities.size() > 1;
            ComponentEditorFunctions<T>::draw_details(context, scene_entity, first_comp, entities, is_multi_edit);
            ImGui::TreePop();
        }

        for (auto& id : entities)
        {
            auto entity = context.ecs_registry.find_by_name(id);
            if (entity.has_value() && entity->template has_component<T>())
            {
                if (remove_component)
                {
                    context.snapshot_manager.prepare_snapshot(STRING_ID(fmt::format("Removed Component {} for {}", glz::type_name<T>, id.string)));
                    if constexpr (HasRemovedFunction<T>) // TODO: should I use this or entt callbacks?
                    {
                        ComponentEditorFunctions<T>::removed(context, entity.value());
                    }

                    entity->remove_component<T>();
                    context.snapshot_manager.commit_snapshot();
                }

                if (reset_values)
                {
                    context.snapshot_manager.prepare_snapshot(STRING_ID(fmt::format("Reset Component {} for {}", glz::type_name<T>, id.string)));
                    if constexpr (HasResetFunction<T>)
                    {
                        ComponentEditorFunctions<T>::reset(context, entity.value());
                    }
                    else
                    {
                        entity->remove_component<T>();
                        entity->add_component<T>();
                        if constexpr (HasAddedFunction<T>) // TODO: should I use this or entt callbacks?
                        {
                            ComponentEditorFunctions<T>::added(context, entity.value());
                        }
                    }
                }
            }
        }

        if (!open)
            imgui::shift_cursor(0.f, -(ImGui::GetStyle().ItemSpacing.y + 1.0f));

        ImGui::PopID();
    }
};
} // portal
