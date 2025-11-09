//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_graph_panel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "portal/engine/settings.h"
#include "portal/engine/imgui/colors.h"
#include "portal/engine/imgui/imgui_extensions.h"
#include "portal/engine/scene/nodes/mesh_node.h"

namespace portal::editor
{

void SceneGraphPanel::on_gui_render()
{
    PORTAL_PROF_ZONE();

    ImGui::Begin("Scene Hierarchy");
    // TODO: check if in runtime
    // TODO: check if project is opened

    if (scene.get_state() != ResourceState::Loaded)
    {
        ImGui::End();
        return;
    }

    static StringId selected_node = INVALID_STRING_ID;

    {
        [[maybe_unused]] auto panel_region = ImGui::GetContentRegionAvail();
        [[maybe_unused]] float line_height = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;

        ImGuiEx::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(3, 3));

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

        for (const auto& node : scene->get_root_nodes())
        {
            bool node_children_open = false;

            if (node->get_id() == selected_node)
            {
                // TODO: color?
                node_children_open = ImGui::TreeNodeEx(reinterpret_cast<void*>(node->get_id().id), flags, node->get_id().string.data());
            }
            else
            {
                node_children_open = ImGui::TreeNodeEx(reinterpret_cast<void*>(node->get_id().id), flags, node->get_id().string.data());
            }

            if (ImGui::IsItemClicked())
            {
                selected_node = node->get_id();
            }

            if (node_children_open)
            {
                // TODO: Child entity system
                ImGui::BulletText("This is a testing text.");
                ImGui::TreePop();
            }
        }

        // Deselect the selected entity if the user clicks in the window but outside of any tree node
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
            selected_node = INVALID_STRING_ID;
        }
    }


    ImGui::End();

    ImGui::Begin("Scene2");
    if (scene.get_state() == ResourceState::Loaded)
    {
        auto draw_node_inner = [](auto& self, const Reference<scene::Node>& node, int& node_id) -> void
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
            if (!node->has_children())
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            ImGui::PushID(node_id++);

            const bool is_mesh = reference_cast<scene::MeshNode>(node) != nullptr;
            if (is_mesh)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
            }

            const bool open = ImGui::TreeNodeEx(node->get_id().string.data(), flags);

            if (is_mesh)
            {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                const auto& translate = glm::vec3(node->get_local_transform()[3]);
                ImGui::Text("Position: %.2f, %.2f, %.2f", translate.x, translate.y, translate.z);
                if (const auto mesh_node = reference_cast<scene::MeshNode>(node))
                {
                    ImGui::Text("Mesh: %s", mesh_node->get_mesh()->get_id().string.data());
                    for (auto& material : mesh_node->get_materials())
                    {
                        ImGui::Text("Material: %s", material->get_id().string.data());
                    }
                }
                ImGui::EndTooltip();
            }

            if (open)
            {
                for (const auto& child : node->get_children())
                {
                    self(self, child, node_id);
                }
                ImGui::TreePop();
            }

            ImGui::PopID();
        };

        ImGui::Text("Scene Graph");
        ImGui::Separator();
        int node_id = 0;

        for (const auto& scene_root : scene->get_root_nodes())
        {
            draw_node_inner(draw_node_inner, scene_root, node_id);
        }
    }
    else
    {
        ImGui::Text("No scene loaded");
    }
    ImGui::End();
}

void SceneGraphPanel::set_scene_context(const ResourceReference<Scene>& new_scene)
{
    scene = new_scene;
}

void SceneGraphPanel::draw_node(const Reference<scene::Node>&) const
{

}

}
