//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "application.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include "portal/core/log.h"
#include "portal/core/debug/assert.h"
#include "portal/engine/settings.h"
#include "../window/window.h"
#include "portal/engine/imgui/im_gui_module.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"
#include "portal/engine/resources/database/folder_resource_database.h"
#include "portal/engine/resources/resources/composite.h"
#include "portal/engine/scene/nodes/mesh_node.h"
#include "portal/engine/window/glfw_window.h"

namespace portal
{
class WindowResizeEvent;
class WindowCloseEvent;

static auto logger = Log::get_logger("Application");

Application::Application(const ApplicationSpecification& spec) : spec(spec)
{
    input = std::make_unique<Input>(
    [this](auto& event)
    {
        on_event(event);
    }
    );


    const WindowProperties window_properties{
        .title = spec.name,
        .extent = {spec.width, spec.width},
    };
    window = make_reference<GlfwWindow>(window_properties, CallbackConsumers{*this, *input});


    vulkan_context = std::make_unique<renderer::vulkan::VulkanContext>();

    auto surface = window->create_surface(*vulkan_context);
    // TODO: find better surface control
    vulkan_context->get_device().add_present_queue(*surface);

    auto swapchain = make_reference<renderer::vulkan::VulkanSwapchain>(*vulkan_context, surface);
    renderer = make_reference<Renderer>(*input, *vulkan_context, swapchain);

    scheduler = std::make_unique<jobs::Scheduler>(spec.scheduler_worker_num);
    reference_manager = std::make_unique<ReferenceManager>();
    resource_database = std::make_unique<FolderResourceDatabase>(spec.resources_path);
    resource_registry = std::make_unique<ResourceRegistry>(*reference_manager, *resource_database, *scheduler, renderer->get_renderer_context());

    engine_context = make_reference<EngineContext>(
        *renderer,
        *resource_registry,
        *window,
        *input
        );

    // TODO: remove this
    imgui_module = std::make_unique<ImGuiModule>(engine_context);

    // TODO: find a better way of subscribing to this
    event_handlers.emplace_back(*window);
    event_handlers.emplace_back(*renderer);
}

Application::~Application()
{
    LOGGER_INFO("Shutting down application");
    vulkan_context->get_device().wait_idle();

    imgui_module.reset();
    engine_context.reset();

    // Then clean up renderer
    renderer.reset();

    // Clean up all loaded resources first
    resource_registry.reset();
    resource_database.reset();
    reference_manager.reset();
    scheduler.reset();

    window.reset();
    vulkan_context.reset();

    glfwTerminate();
}

void Application::run()
{
    try
    {
        // TODO: Remove from here
        [[maybe_unused]] auto composite = engine_context->get_resource_registry().immediate_load<Composite>(STRING_ID("game/ABeautifulGame"));
        auto scene = engine_context->get_resource_registry().get<Scene>(STRING_ID("game/gltf-Scene-Scene"));
        PORTAL_ASSERT(scene.get_state() == ResourceState::Loaded, "Failed to load scene");

        should_stop.clear();

        LOGGER_INFO("Starting application");
        while (!should_stop.test())
        {
            // Process Events
            {
                // Process input events
                window->process_events();
                // Run queues events
            }

            // Run single iteration of renderer::draw
            {
                renderer->begin_frame();
                imgui_module->begin();

                {
                    // TODO: put in layer
                    imgui_module->on_gui_render();

                    renderer->update_scene(time_step, scene);

                    renderer->update_imgui(time_step);
                    ImGui::Begin("Scene");
                    if (scene.get_state() == ResourceState::Loaded)
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

                        for (const auto& scene_root : scene->get_root_nodes())
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

                {
                    // TODO: put in layer ?
                    renderer->draw_geometry();
                    imgui_module->end();
                }

                renderer->end_frame();


                current_frame_count = (current_frame_count + 1) % renderer->get_frames_in_flight();
                PORTAL_FRAME_MARK();
            }

            const auto time = static_cast<float>(glfwGetTime());
            frame_time = time - last_frame_time;
            time_step = glm::min<float>(frame_time, 0.0333f);
            last_frame_time = time;
        }
        LOGGER_INFO("Application stopped");
    }
    catch (const std::exception& e)
    {
        LOG_FATAL("Exception caught: {}", e.what());
    }
    catch (...)
    {
        LOG_FATAL("Fatal unknown exception caught");
    }
}

void Application::stop()
{
    should_stop.test_and_set();
}

void Application::on_init()
{
}

void Application::on_shutdown()
{
}

void Application::on_update(float)
{
}

void Application::on_event(Event& event)
{
    for (auto& handler : event_handlers)
        handler.get().on_event(event);
}

void Application::add_module(std::shared_ptr<Module>)
{
}

void Application::remove_module(std::shared_ptr<Module>)
{
}

void Application::render_gui()
{
}

void Application::on_resize(const WindowExtent extent)
{
    if (extent.width == 0 || extent.height == 0)
        return;

    const auto glfw_window = reference_cast<GlfwWindow>(window);
    auto [width, height] = glfw_window->resize(extent);

    renderer->on_resize(width, height);
}

void Application::on_focus(const bool set_focused)
{
    focused = set_focused;
}

void Application::on_close()
{
    should_stop.test_and_set();
}


} // portal
