//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "panel_manager.h"

#include <entt/entt.hpp>
#include <utility>
#include <portal/third_party/imgui/ImGuiNotify.h>


#include "selection_system.h"
#include "portal/engine/engine_context.h"
#include "portal/serialization/archive/json_archive.h"

namespace portal
{
void PanelData::archive(ArchiveObject& object) const
{
    const auto name_id = STRING_ID(name);
    object.add_property("id", id);
    object.add_property("name", name_id);
    object.add_property("open", open);
}

PanelData PanelData::dearchive(ArchiveObject& object)
{
    StringId name_id;
    PanelData panel;
    object.get_property("id", panel.id);
    object.get_property("name", name_id);
    object.get_property("open", panel.open);
    panel.name = name_id.string.data();
    return panel;
}

PanelManager::PanelManager(std::filesystem::path state_path) : state_path(std::move(state_path))
{
    load_state();
}

PanelData* PanelManager::get_panel_data(const StringId& id)
{
    for (auto& map : panels)
    {
        if (map.contains(id))
            return &map.at(id);
    }
    return nullptr;
}

const PanelData* PanelManager::get_panel_data(const StringId& id) const
{
    for (auto& map : panels)
    {
        if (map.contains(id))
            return &map.at(id);
    }
    return nullptr;
}

void PanelManager::remove_panel(StringId id)
{
    for (auto& map : panels)
    {
        if (map.contains(id))
        {
            map.erase(id);
            return;
        }
    }
}

void PanelManager::save_state()
{
    JsonArchive json_archive;

    std::vector<PanelData> panel_data;
    for (auto& map : panels)
    {
        for (auto& panel : map | std::views::values)
        {
            panel_data.push_back(panel);
        }
    }
    json_archive.add_property("panels", panel_data);

    json_archive.dump(state_path);
}

void PanelManager::load_state()
{
    JsonArchive json_archive;
    json_archive.read(state_path);

    std::vector<PanelData> panel_data;
    json_archive.get_property("panels", panel_data);

    for (auto& panel : panel_data)
    {
        auto* data = get_panel_data(panel.id);
        if (data == nullptr)
            break;

        data->open = panel.open;
    }
}

void PanelManager::on_gui_render(EditorContext& editor_context, FrameContext& frame)
{
    for (auto& panel_map : panels)
    {
        for (PanelData& panel_data : panel_map | std::views::values)
        {
            bool closed_this_frame = false;

            if (panel_data.open)
            {
                panel_data.panel->on_gui_render(editor_context, frame, panel_data.open);
                closed_this_frame = !panel_data.open;
            }

            if (closed_this_frame)
            {
                panel_data.panel->on_close();
                save_state();
            }
        }
    }
}

std::unordered_map<StringId, PanelData>& PanelManager::get_panels(const PanelMenuCategory category)
{
    return panels[std::to_underlying(category)];
}

const std::unordered_map<StringId, PanelData>& PanelManager::get_panels(const PanelMenuCategory category) const
{
    return panels[std::to_underlying(category)];
}

// struct SceneContext;

// void draw_node(
//     Entity entity,
//     Entity scope,
//     int& node_id,
//     const RelationshipComponent& relationship,
//     const NameComponent& name
// )
// {
//     ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
//     if (relationship.children == 0)
//     {
//         flags |= ImGuiTreeNodeFlags_Leaf;
//     }
//
//     const bool is_selected = SelectionSystem::is_selected(entity, scope);
//     if (is_selected)
//         flags |= ImGuiTreeNodeFlags_Selected;
//
//
//     ImGui::PushID(node_id++);
//
//     const bool is_mesh = entity.has_component<StaticMeshComponent>();
//     if (is_mesh)
//     {
//         ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
//     }
//
//     const bool open = ImGui::TreeNodeEx(name.name.string.data(), flags);
//
//     if (is_mesh)
//     {
//         ImGui::PopStyleColor();
//     }
//
//     if (ImGui::IsItemHovered())
//     {
//         ImGui::BeginTooltip();
//         if (entity.has_component<TransformComponent>())
//         {
//             auto& transform = entity.get_component<TransformComponent>();
//             const auto& translate = glm::vec3(transform.get_world_matrix()[3]);
//             ImGui::Text("Position: %.2f, %.2f, %.2f", translate.x, translate.y, translate.z);
//         }
//
//         if (is_mesh)
//         {
//             auto& mesh = entity.get_component<StaticMeshComponent>();
//
//             ImGui::Text("Mesh: %s", mesh.mesh->get_id().string.data());
//             for (auto& material : mesh.materials)
//             {
//                 ImGui::Text("Material: %s", material->get_id().string.data());
//             }
//         }
//         ImGui::EndTooltip();
//     }
//
//     if (ImGui::IsItemClicked())
//     {
//         SelectionSystem::deselect_all(scope);
//         SelectionSystem::select(entity, scope);
//     }
//
//     if (open)
//     {
//         for (const auto& child : entity.children())
//         {
//             auto& child_rel = child.get_component<RelationshipComponent>();
//             auto& child_name = child.get_component<NameComponent>();
//             draw_node(child, scope, node_id, child_rel, child_name);
//         }
//         ImGui::TreePop();
//     }
//
//     ImGui::PopID();
// }

//
// void PanelManager::print_scene_graph(ecs::Registry& registry, const FrameContext& frame)
// {
//     const auto scene = std::any_cast<SceneContext>(&frame.scene_context)->active_scene;
//
//     const auto relationship_group = registry.group<NameComponent>(entt::get<RelationshipComponent, TransformComponent>);
//     relationship_group.sort(
//         [&registry](const entt::entity lhs_raw, const entt::entity rhs_raw)
//         {
//             auto lhs = registry.entity_from_id(lhs_raw);
//             auto rhs = registry.entity_from_id(rhs_raw);
//
//             const auto& [left_name, _] = lhs.get_component<NameComponent>();
//             const auto& [right_name, __] = rhs.get_component<NameComponent>();
//
//             return left_name.string < right_name.string;
//         }
//     );
//
//     bool show_changes = static_cast<bool>(scene.get_dirty() & ResourceDirtyBits::StateChange);
//
//     std::string window_title = show_changes
//                                    ? "Scene*###Scene"
//                                    : "Scene###Scene";
//     ImGui::Begin(window_title.c_str());
//
//     ImGui::Text("Scene Graph");
//     ImGui::Separator();
//     int node_id = 0;
//
//     auto scene_entity = scene->get_scene_entity();
//     for (auto child : scene_entity.children())
//     {
//         auto& relationship = child.get_component<RelationshipComponent>();
//         auto& name_comp = child.get_component<NameComponent>();
//         draw_node(child, scene->get_scene_entity(), node_id, relationship, name_comp);
//     }
//     ImGui::End();
// }
//
// void PanelManager::print_controls(ecs::Registry&)
// {
//     // TODO: This A - should not exist B - Should not be here, A necessary evil for now
//     ImGui::Begin("Controls");
//     ImGui::Text("RMB - Enter Movement Mode");
//     ImGui::Separator();
//     ImGui::Text("W - Move Forwards");
//     ImGui::Text("S - Move Backwards");
//     ImGui::Text("A - Move Left");
//     ImGui::Text("D - Move Right");
//     ImGui::Text("E - Move Up");
//     ImGui::Text("Q - Move Down");
//     ImGui::End();
// }
} // portal
