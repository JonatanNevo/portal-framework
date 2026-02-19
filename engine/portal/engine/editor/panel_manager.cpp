//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "panel_manager.h"

#include <imgui.h>
#include <entt/entt.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <portal/third_party/imgui/ImGuiNotify.h>


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
    const NameComponent& name
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
        if (entity.has_component<TransformComponent>())
        {
            auto& transform = entity.get_component<TransformComponent>();
            const auto& translate = glm::vec3(transform.get_world_matrix()[3]);
            ImGui::Text("Position: %.2f, %.2f, %.2f", translate.x, translate.y, translate.z);
        }

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
            draw_node(child, scope, node_id, child_rel, child_name);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void PanelManager::on_gui_render(EditorContext& editor_context, FrameContext& frame)
{
    for (const auto& panel : panels)
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

    auto scene_entity = scene->get_scene_entity();
    for (auto child : scene_entity.children())
    {
        auto& relationship = child.get_component<RelationshipComponent>();
        auto& name_comp = child.get_component<NameComponent>();
        draw_node(child, scene->get_scene_entity(), node_id, relationship, name_comp);
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

void PanelManager::print_stats_block(ecs::Registry&, const FrameContext& frame)
{
    static constexpr int history_size = 256;

    static std::array<float, history_size> fps_history = {};
    static std::array<float, history_size> frame_time_history_ms = {};
    static std::array<float, history_size> draw_time_history_ms = {};
    static std::array<float, history_size> update_time_history_ms = {};
    static int history_index = 0;

    fps_history[history_index] =  1000.f / frame.stats.frame_time;
    frame_time_history_ms[history_index] = frame.stats.frame_time;
    draw_time_history_ms[history_index] = frame.stats.mesh_draw_time;
    update_time_history_ms[history_index] = frame.stats.scene_update_time;

    history_index = (history_index + 1) % history_size;

    const auto avg = [](const auto& arr) -> float
    {
        return std::ranges::fold_left(arr, 0.f, std::plus<float>()) / static_cast<float>(arr.size());
    };

    ImGui::Begin("Stats");
    ImGui::Text("FPS %.2f", avg(fps_history));
    ImGui::Text("frame time %.3f ms", avg(frame_time_history_ms));
    ImGui::Text("draw time %.3f ms", avg(draw_time_history_ms));
    ImGui::Text("update time %.3f ms", avg(update_time_history_ms));
    ImGui::Text("triangles %i", frame.stats.triangle_count);
    ImGui::Text("draws %i", frame.stats.drawcall_count);
    ImGui::End();
}
} // portal
