//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_graph_panel.h"

#include <imgui.h>

#include "portal/engine/components/relationship.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/editor/selection_system.h"
#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/engine/imgui/tree_node_with_icon.h"
#include "portal/engine/imgui/widgets/search_widget.h"
#include "portal/engine/scene/scene_context.h"
#include "portal/input/input_manager.h"

namespace portal
{
struct SceneGraphConsts
{
    float edge_offset = 4.f;
    float header_row_height = 22.f;
};

void SceneGraphPanel::on_gui_render(EditorContext& editor_context, [[maybe_unused]] FrameContext& frame_context, bool& is_open)
{
    constexpr SceneGraphConsts consts;

    {
        imgui::ScopedStyle padding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Scene Graph", &is_open);
    }

    window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    ImRect window_rect = {ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax()};

    {
        imgui::shift_cursor(consts.edge_offset * 3.f, consts.edge_offset * 2.f);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - consts.edge_offset * 3.0f);

        if (activate_search_widget)
        {
            ImGui::SetKeyboardFocusHere();
            activate_search_widget = false;
        }

        imgui::search_widget(editor_context, search_string);

        ImGui::Spacing();

        // Entity Tree
        imgui::ScopedStyle cell_padding(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 0.0f));
        auto table_background_color = editor_context.theme.scoped_color(ImGuiCol_TableRowBgAlt, imgui::ThemeColors::Background2);

        // Table
        {
            auto background_color = editor_context.theme.scoped_color(ImGuiCol_ChildBg, imgui::ThemeColors::Background1);
            auto table_flags = ImGuiTableFlags_NoPadInnerX
                | ImGuiTableFlags_Resizable
                | ImGuiTableFlags_Reorderable
                | ImGuiTableFlags_ScrollY;

            constexpr int num_columns = 2;
            if (ImGui::BeginTable("scenegraph-table", num_columns, table_flags, ImVec2(ImGui::GetContentRegionAvail())))
            {
                ImGui::TableSetupColumn("Label");
                ImGui::TableSetupColumn("Type");

                // Headers
                {
                    auto header_hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Primary1);
                    auto header_active = editor_context.theme.scoped_color(ImGuiCol_HeaderActive, imgui::ThemeColors::Primary1);

                    ImGui::TableSetupScrollFreeze(ImGui::TableGetColumnCount(), 1);

                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers, consts.header_row_height);
                    for (int column = 0; column < num_columns; column++)
                    {
                        ImGui::TableSetColumnIndex(column);
                        const char* name = ImGui::TableGetColumnName(column);
                        imgui::ScopedID id(column);

                        imgui::shift_cursor(consts.edge_offset * 3.f, consts.edge_offset * 2.f);
                        ImGui::TableHeader(name);
                        imgui::shift_cursor(-consts.edge_offset * 3.f, -consts.edge_offset * 2.f);
                    }
                    // ImGui::SetCursorPosX(ImGui::GetCurrentTable()->OuterRect.Min.x);
                    imgui::underline(editor_context.theme[imgui::ThemeColors::Background3], true, 0.f, 5.f);
                }

                // List
                {
                    imgui::ScopedColor disable_header(ImGuiCol_Header, IM_COL32_DISABLE);
                    imgui::ScopedColor disable_hovered(ImGuiCol_HeaderHovered, IM_COL32_DISABLE);
                    imgui::ScopedColor disable_active(ImGuiCol_HeaderActive, IM_COL32_DISABLE);

                    auto* scene_context = std::any_cast<SceneContext>(&frame_context.scene_context);

                    auto scene = scene_context->active_scene;
                    const auto relationship_group = frame_context.ecs_registry->group<NameComponent>(
                        entt::get<RelationshipComponent, TransformComponent>
                    );
                    relationship_group.sort(
                        [&registry = frame_context.ecs_registry](const entt::entity lhs_raw, const entt::entity rhs_raw)
                        {
                            auto lhs = registry->entity_from_id(lhs_raw);
                            auto rhs = registry->entity_from_id(rhs_raw);

                            const auto& [left_name, _] = lhs.get_component<NameComponent>();
                            const auto& [right_name, __] = rhs.get_component<NameComponent>();

                            return left_name.string < right_name.string;
                        }
                    );

                    for (auto entity : scene->get_scene_entity().children())
                    {
                        draw_entity_node(editor_context, entity, scene->get_scene_entity());
                    }
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Spacing();
                ImGui::Dummy(ImVec2(0, 50.0f));

                ImGui::EndTable();
            }
        }
    }

    ImGui::End();
}

struct DrawEntityNodeConsts
{
    float edge_offset = 4.f;
    float row_height = 21.f;
    float text_base_offset = 3.f;
};

void SceneGraphPanel::draw_entity_node(EditorContext& editor_context, const Entity& entity, const Entity& scene_entity)
{
    constexpr DrawEntityNodeConsts consts;

    auto name = STRING_ID("Unnamed Entity");
    if (entity.has_component<NameComponent>())
        name = entity.get_component<NameComponent>().name;

    constexpr auto max_search_depth = 20;
    bool has_children_matching_search = name_search_recursive(entity, max_search_depth);

    if (!imgui::is_matching_search(name.string.data(), search_string) && !has_children_matching_search)
        return;

    auto* window = ImGui::GetCurrentWindow();
    window->DC.CurrLineSize.y = consts.row_height;
    //---------------------------------------------
    ImGui::TableNextRow(0, consts.row_height);

    // Label column
    //-------------
    ImGui::TableNextColumn();

    window->DC.CurrLineTextBaseOffset = consts.text_base_offset;

    const ImVec2 row_area_min = ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), 0).Min;
    const ImVec2 row_area_max = {
        ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), ImGui::TableGetColumnCount() - 1).Max.x - 20,
        row_area_min.y + consts.row_height
    };

    bool is_selected = SelectionSystem::is_selected(name, scene_entity);

    auto flags = (is_selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_AllowOverlap |
        ImGuiTreeNodeFlags_SpanAvailWidth;
    if (has_children_matching_search)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (!entity.has_children())
        flags |= ImGuiTreeNodeFlags_Leaf;

    auto name_id = ImGui::GetID(name.string.data());

    ImGui::PushClipRect(row_area_min, row_area_max, false);
    ImGui::ItemAdd(ImRect(row_area_min, row_area_max), name_id);
    bool is_hovered, is_held;
    bool is_row_clicked = ImGui::ButtonBehavior(
        ImRect(row_area_min, row_area_max),
        name_id,
        &is_hovered,
        &is_held,
        static_cast<int>(ImGuiButtonFlags_PressedOnClickRelease) | ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight
    );
    bool was_row_right_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    ImGui::PopClipRect();

    // Drag & Drop
    //------------
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        auto& selected_entities = SelectionSystem::get_selections(scene_entity);
        if (!SelectionSystem::is_selected(name, scene_entity))
        {
            ImGui::TextUnformatted(name.string.data());
            ImGui::SetDragDropPayload("scene_entity_hierarchy", &name, sizeof(StringId)); // TODO: should I set the entire entity as a payload?
        }
        else
        {
            for (const auto& selected_entity : selected_entities)
            {
                ImGui::TextUnformatted(selected_entity.string.data());
            }
            ImGui::SetDragDropPayload("scene_entity_hierarchy", selected_entities.data(), selected_entities.size() * sizeof(StringId));
            // TODO: should I set the entire entity as a payload?
        }

        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        const auto* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

        if (payload)
        {
            std::span payload_span{static_cast<StringId*>(payload->Data), payload->DataSize / sizeof(StringId)};

            for (auto& payload_name : payload_span)
            {
                auto dropped_entity = editor_context.ecs_registry.find_by_name(payload_name);
                if (!dropped_entity.has_value())
                    continue;

                dropped_entity->set_parent(entity);
            }
        }

        ImGui::EndDragDropTarget();
    }

    // Row colouring
    //--------------

    auto check_descendants_selected = [&](Entity ent, auto self) -> bool
    {
        if (SelectionSystem::is_selected(ent.get_component<NameComponent>().name, scene_entity))
            return true;

        if (ent.has_children())
        {
            for (auto child : ent.children())
            {
                if (self(child, self))
                    return true;
            }
        }

        return false;
    };

    auto fill_row_with_color = [](const ImColor& color)
    {
        for (int column = 0; column < ImGui::TableGetColumnCount(); column++)
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color, column);
    };

    if (is_selected)
    {
        if (window_focused || imgui::navigated_to())
        {
            fill_row_with_color(editor_context.theme[imgui::ThemeColors::Primary1]);
        }
        else
        {
            const auto col = imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Primary1], 0.9f);
            fill_row_with_color(imgui::color_with_multiplied_saturation(col, 0.7f));
        }
    }
    else if (is_hovered)
    {
        fill_row_with_color(editor_context.theme[imgui::ThemeColors::Primary2]);
    }
    else if (check_descendants_selected(entity, check_descendants_selected))
    {
        fill_row_with_color(editor_context.theme[imgui::ThemeColors::Secondary2]);
    }

    // Tree node
    //----------
    auto& g = *ImGui::GetCurrentContext();
    auto& style = ImGui::GetStyle();

    const ImVec2 label_size = ImGui::CalcTextSize(name.string.data(), nullptr, false);
    const ImVec2 padding = ((flags & ImGuiTreeNodeFlags_FramePadding))
                               ? style.FramePadding
                               : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));
    const float text_offset_x = g.FontSize + padding.x * 2;                          // Collapser arrow width + Spacing
    const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset); // Latch before ItemSize changes it
    const ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
    const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
    const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
    const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);


    auto node_id = fmt::format("{}##node", name.string);
    bool previous_state = ImGui::TreeNodeUpdateNextOpen(ImGui::GetID(node_id.c_str()), flags);

    if (is_mouse_x_over_arrow && is_row_clicked)
        ImGui::SetNextItemOpen(!previous_state);

    if (!is_selected && check_descendants_selected(entity, check_descendants_selected))
        ImGui::SetNextItemOpen(true);


    // TODO: add icon based on node type
    const auto opened = tree_node_with_icon(nullptr, nullptr, ImGui::GetID(node_id.c_str()), flags, name.string.data(), nullptr);

    const auto row_index = ImGui::TableGetRowIndex();
    if (row_index >= first_selected_row && row_index <= last_selected_row && !SelectionSystem::is_selected(name, scene_entity) &&
        shift_selection_running)
    {
        SelectionSystem::select(name, scene_entity);
        if (SelectionSystem::selection_count(scene_entity) == (last_selected_row - first_selected_row) + 1)
            shift_selection_running = false;
    }

    auto right_click_popup_id = fmt::format("{}_context_menu", name.string);

    if (ImGui::BeginPopupContextItem(right_click_popup_id.c_str()))
    {
        // TODO: handle delete popup
        ImGui::EndPopup();
    }

    // Selection
    //------------
    if (is_row_clicked)
    {
        if (was_row_right_clicked)
        {
            ImGui::OpenPopup(right_click_popup_id.c_str());
        }
        else
        {
            bool ctrl_down = editor_context.input_manager.is_key_pressed(Key::LeftControl) || editor_context.input_manager.is_key_pressed(
                Key::RightControl
            );
            bool shift_down = editor_context.input_manager.is_key_pressed(Key::LeftShift) || editor_context.input_manager.is_key_pressed(
                Key::RightShift
            );
            if (shift_down && SelectionSystem::selection_count(scene_entity) > 0)
            {
                SelectionSystem::deselect_all(scene_entity);

                if (row_index < first_selected_row)
                {
                    last_selected_row = first_selected_row;
                    first_selected_row = row_index;
                }
                else
                {
                    last_selected_row = row_index;
                }

                shift_selection_running = true;
            }
            else if (!ctrl_down || shift_down)
            {
                SelectionSystem::deselect_all(scene_entity);
                SelectionSystem::select(name, scene_entity);
                first_selected_row = row_index;
                last_selected_row = -1;
            }
            else
            {
                if (is_selected)
                    SelectionSystem::deselect(name, scene_entity);
                else
                    SelectionSystem::select(name, scene_entity);
            }
        }

        ImGui::FocusWindow(ImGui::GetCurrentWindow());
    }

    ImGui::TableNextColumn();
    // TODO: write type name

    // Draw Children
    //--------------

    if (opened)
    {
        for (auto child : entity.children())
            draw_entity_node(editor_context, child, scene_entity);

        ImGui::TreePop();
    }
}

bool SceneGraphPanel::name_search_recursive(const Entity& entity, const uint32_t search_depth, const uint32_t current_depth)
{
    if (search_string.empty())
        return false;

    for (auto child : entity.children())
    {
        if (child.has_component<NameComponent>())
        {
            if (imgui::is_matching_search(child.get_component<NameComponent>().name.string, search_string))
                return true;
        }

        const bool found = name_search_recursive(child, search_depth, current_depth + 1);
        if (found)
            return true;
    }

    return false;
}
} // portal

