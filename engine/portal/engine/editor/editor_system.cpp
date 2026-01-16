//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "editor_system.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "selection_manager.h"
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/mesh.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/scene/scene.h"

namespace portal
{
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

void EditorGuiSystem::execute(ecs::Registry& registry, FrameContext& frame)
{
    print_scene_graph(registry, frame);
    print_controls(registry);
    print_stats_block(registry, frame);
    print_details_panel(registry, frame);
}

void EditorGuiSystem::print_scene_graph(ecs::Registry& registry, FrameContext& frame)
{
    const auto relationship_group = group(registry);
    relationship_group.sort(
        [&registry](const entt::entity lhs_raw, const entt::entity rhs_raw)
        {
            auto lhs = registry.entity_from_id(lhs_raw);
            auto rhs = registry.entity_from_id(rhs_raw);

            const auto& [left_name] = lhs.get_component<NameComponent>();
            const auto& [right_name] = rhs.get_component<NameComponent>();

            return left_name.string < right_name.string;
        }
    );

    ImGui::Begin("Scene");

    ImGui::Text("Scene Graph");
    ImGui::Separator();
    int node_id = 0;

    for (auto&& [entity, name_comp, relationship, transform] : relationship_group.each())
    {
        // Iterating only on the root entities
        if (relationship.parent != null_entity)
            continue;

        draw_node(registry.entity_from_id(entity), frame.active_scene->get_scene_entity(), node_id, relationship, name_comp, transform);
    }
    ImGui::End();
}

void EditorGuiSystem::print_controls(ecs::Registry&)
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

void EditorGuiSystem::print_stats_block(ecs::Registry&, FrameContext& frame)
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

void show_transform_controls(Scene&, Entity entity, TransformComponent& transform)
{
    ImGui::Separator();

    float ptr_translation[3], ptr_rotation[3], ptr_scale[3];

    std::memcpy(ptr_translation, glm::value_ptr(transform.get_translation()), sizeof(float) * 3);
    std::memcpy(ptr_rotation, glm::value_ptr(transform.get_rotation_euler()), sizeof(float) * 3);
    std::memcpy(ptr_scale, glm::value_ptr(transform.get_scale()), sizeof(float) * 3);

    ImGui::InputFloat3("Tr", ptr_translation);
    ImGui::InputFloat3("Rt", ptr_rotation);
    ImGui::InputFloat3("Sc", ptr_scale);

    transform.set_translation(glm::vec3{ptr_translation[0], ptr_translation[1], ptr_translation[2]});
    transform.set_rotation_euler(glm::vec3{ptr_rotation[0], ptr_rotation[1], ptr_rotation[2]});
    transform.set_scale(glm::vec3{ptr_scale[0], ptr_scale[1], ptr_scale[2]});

    entity.patch_component<TransformComponent>([transform](TransformComponent& comp)
    {
        comp.set_translation(transform.get_translation());
        comp.set_rotation_euler(transform.get_rotation_euler());
        comp.set_scale(transform.get_scale());
    });
}

void show_camera_component(Entity entity, CameraComponent& camera)
{
    ImGui::Separator();
    auto& controller = entity.get_component<BaseCameraController>();

    if (entity.has_component<MainCameraTag>()) ImGui::Text("Main Camera");
    ImGui::InputFloat3("Direction", glm::value_ptr(controller.forward_direction));

    ImGui::SliderFloat("Camera Speed", &controller.speed, 0.1f, 10.0f);
    ImGui::InputFloat("Near Clip", &camera.near_clip);
    ImGui::InputFloat("Far Clip", &camera.far_clip);
    ImGui::InputFloat("FOV", &camera.vertical_fov);
}

void EditorGuiSystem::print_details_panel(ecs::Registry&, const FrameContext& frame)
{
    ImGui::Begin("Details");
    if (SelectionSystem::has_selection(frame.active_scene->get_scene_entity()))
    {
        auto selected_entity = SelectionSystem::get_selected_entity(frame.active_scene->get_scene_entity());

        ImGui::Text("%s Details", selected_entity.get_name().string.data());

        if (selected_entity.has_component<TransformComponent>())
            show_transform_controls(*frame.active_scene, selected_entity, selected_entity.get_component<TransformComponent>());

        if (selected_entity.has_component<CameraComponent>())
            show_camera_component(selected_entity, selected_entity.get_component<CameraComponent>());
    }
    else
    {
        ImGui::Text("No entity selected");
    }
    ImGui::End();
}
} // portal
