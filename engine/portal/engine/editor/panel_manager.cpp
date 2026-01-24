//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "panel_manager.h"

#include <imgui.h>
#include <entt/entt.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "selection_manager.h"
#include "portal/engine/engine_context.h"
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/mesh.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/scene/scene.h"
#include "portal/engine/scene/scene_context.h"

namespace portal
{
struct SceneContext;

void draw_node(
    Entity entity,
    Entity scope,
    int& node_id,
    const RelationshipComponent& relationship,
    const NameComponent& name,
    const TransformComponent& transform
)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (relationship.children == 0)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const bool is_selected = SelectionSystem::is_selected(entity, scope);
    if (is_selected)
        flags |= ImGuiTreeNodeFlags_Selected;


    ImGui::PushID(node_id++);

    const bool is_mesh = entity.has_component<StaticMeshComponent>();
    if (is_mesh)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
    }

    const bool open = ImGui::TreeNodeEx(name.name.string.data(), flags);

    if (is_mesh)
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        const auto& translate = glm::vec3(transform.get_world_matrix()[3]);
        ImGui::Text("Position: %.2f, %.2f, %.2f", translate.x, translate.y, translate.z);

        if (is_mesh)
        {
            auto& mesh = entity.get_component<StaticMeshComponent>();

            ImGui::Text("Mesh: %s", mesh.mesh->get_id().string.data());
            for (auto& material : mesh.materials)
            {
                ImGui::Text("Material: %s", material->get_id().string.data());
            }
        }
        ImGui::EndTooltip();
    }

    if (ImGui::IsItemClicked())
    {
        SelectionSystem::select(entity, scope);
    }

    if (open)
    {
        for (const auto& child : entity.children())
        {
            auto& child_rel = child.get_component<RelationshipComponent>();
            auto& child_name = child.get_component<NameComponent>();
            auto& child_transform = child.get_component<TransformComponent>();
            draw_node(child, scope, node_id, child_rel, child_name, child_transform);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void PanelManager::on_gui_render(EditorContext& editor_context, FrameContext& frame)
{
    for (const auto& panel: panels)
    {
        panel->on_gui_render(editor_context, frame);
    }

    print_scene_graph(*frame.ecs_registry, frame);
    print_controls(*frame.ecs_registry);
    print_stats_block(*frame.ecs_registry, frame);
}

void PanelManager::print_scene_graph(ecs::Registry& registry, const FrameContext& frame)
{
    const auto scene = std::any_cast<SceneContext>(&frame.scene_context)->active_scene;

    const auto relationship_group = registry.group<NameComponent>(entt::get<RelationshipComponent, TransformComponent>);
    relationship_group.sort(
        [&registry](const entt::entity lhs_raw, const entt::entity rhs_raw)
        {
            auto lhs = registry.entity_from_id(lhs_raw);
            auto rhs = registry.entity_from_id(rhs_raw);

            const auto& [left_name, _] = lhs.get_component<NameComponent>();
            const auto& [right_name, __] = rhs.get_component<NameComponent>();

            return left_name.string < right_name.string;
        }
    );

    bool show_changes = static_cast<bool>(scene.get_dirty() & ResourceDirtyBits::StateChange);

    std::string window_title = show_changes
                                   ? "Scene*###Scene"
                                   : "Scene###Scene";
    ImGui::Begin(window_title.c_str());

    ImGui::Text("Scene Graph");
    ImGui::Separator();
    int node_id = 0;

    for (auto&& [entity, name_comp, relationship, transform] : relationship_group.each())
    {
        // Iterating only on the root entities
        if (relationship.parent != null_entity)
            continue;

        draw_node(registry.entity_from_id(entity), scene->get_scene_entity(), node_id, relationship, name_comp, transform);
    }
    ImGui::End();
}

void PanelManager::print_controls(ecs::Registry&)
{
    // TODO: This A - should not exist B - Should not be here, A necessary evil for now
    ImGui::Begin("Controls");
    ImGui::Text("RMB - Enter Movement Mode");
    ImGui::Separator();
    ImGui::Text("W - Move Forwards");
    ImGui::Text("S - Move Backwards");
    ImGui::Text("A - Move Left");
    ImGui::Text("D - Move Right");
    ImGui::Text("E - Move Up");
    ImGui::Text("Q - Move Down");
    ImGui::End();
}

void PanelManager::print_stats_block(ecs::Registry&, FrameContext& frame)
{
    static std::array<float, 100> fps_history = {};
    static int fps_history_index = 0;

    fps_history[fps_history_index] = 1000.f / frame.stats.frame_time;
    fps_history_index = (fps_history_index + 1) % fps_history.size();

    ImGui::Begin("Stats");
    ImGui::Text("FPS %f", std::ranges::fold_left(fps_history, 0.f, std::plus<float>()) / 100.f);
    ImGui::Text("frametime %f ms", frame.stats.frame_time);
    ImGui::Text("draw time %f ms", frame.stats.mesh_draw_time);
    ImGui::Text("update time %f ms", frame.stats.scene_update_time);
    ImGui::Text("triangles %i", frame.stats.triangle_count);
    ImGui::Text("draws %i", frame.stats.drawcall_count);
    ImGui::End();
}


} // portal
