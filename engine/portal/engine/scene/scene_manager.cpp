//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_manager.h"

#include <imgui.h>

#include "portal/engine/events/window_events.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/scene/nodes/mesh_node.h"
#include "portal/input/input_events.h"


namespace portal
{
SceneManager::SceneManager(ModuleStack& stack) : TaggedModule(stack, STRING_ID("Scene Manager")), camera(get_dependency<InputManager>())
{
    const auto& renderer = get_dependency<Renderer>();
    auto& swapchain = renderer.get_swapchain();
    camera.on_resize(static_cast<uint32_t>(swapchain.get_width()), static_cast<uint32_t>(swapchain.get_height()));
}

void SceneManager::set_active_scene(const ResourceReference<Scene>& new_scene)
{
    active_scene = new_scene;
}

void SceneManager::update(FrameContext& frame)
{
    PORTAL_PROF_ZONE();

    auto* rendering_context = std::any_cast<renderer::FrameRenderingContext>(&frame.rendering_context);

    const auto start = std::chrono::system_clock::now();
    camera.update(frame.delta_time);

    const glm::mat4 view = camera.get_view();
    glm::mat4 projection = camera.get_projection();
    // invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
    projection[1][1] *= -1;

    rendering_context->scene_data.view = view;
    rendering_context->scene_data.proj = projection;
    rendering_context->scene_data.view_proj = projection * view;

    active_scene->draw(glm::mat4{1.f}, frame);

    //some default lighting parameters
    rendering_context->scene_data.ambient_color = glm::vec4(.1f);
    rendering_context->scene_data.sunlight_color = glm::vec4(1.f);
    rendering_context->scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);

    const auto end = std::chrono::system_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    frame.stats.scene_update_time = elapsed.count() / 1000.f;
}

void SceneManager::gui_update(FrameContext&)
{
    ImGui::Begin("Camera");
    ImGui::Text("position %f %f %f", camera.get_position().x, camera.get_position().y, camera.get_position().z);
    ImGui::Text("direction %f %f %f", camera.get_direction().x, camera.get_direction().y, camera.get_direction().z);

    // Input to control camera speed
    float camera_speed = camera.get_speed();
    if (ImGui::SliderFloat("Camera Speed", &camera_speed, 0.1f, 10.0f))
    {
        camera.set_speed(camera_speed);
    }
    ImGui::End();

    print_scene_graph();
}


void SceneManager::on_event(Event& event)
{
    EventRunner runner(event);
    runner.run_on<KeyPressedEvent>(
        [&](const auto& e)
        {
            camera.on_key_down(e.get_key());
            return false;
        }
    );
    runner.run_on<KeyReleasedEvent>(
        [&](const auto& e)
        {
            camera.on_key_up(e.get_key());
            return false;
        }
    );
    runner.run_on<MouseMovedEvent>(
        [&](const auto& e)
        {
            camera.on_mouse_move(e.get_position());
            return false;
        }
    );
    runner.run_on<WindowResizeEvent>(
        [&](const auto& e)
        {
            on_resize(e);
            return false;
        }
    );
}

void SceneManager::on_resize(const WindowResizeEvent& event)
{
    camera.on_resize(static_cast<uint32_t>(event.get_width()), static_cast<uint32_t>(event.get_height()));
}

void SceneManager::print_scene_graph()
{
    ImGui::Begin("Scene");
    if (active_scene.get_state() == ResourceState::Loaded)
    {
        auto draw_node = [](auto& self, const Reference<scene::Node>& node, int& node_id) -> void
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

        for (const auto& scene_root : active_scene->get_root_nodes())
        {
            draw_node(draw_node, scene_root, node_id);
        }
    }
    else
    {
        ImGui::Text("No scene loaded");
    }
    ImGui::End();
}
} // portal
