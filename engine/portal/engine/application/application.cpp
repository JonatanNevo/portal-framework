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
#include "portal/engine/application/window.h"
#include "portal/engine/imgui/im_gui_module.h"
#include "portal/engine/renderer/vulkan/vulkan_window.h"
#include "portal/engine/resources/database/folder_resource_database.h"
#include "portal/engine/scene/nodes/mesh_node.h"

namespace portal
{
class WindowResizeEvent;
class WindowCloseEvent;

static auto logger = Log::get_logger("Application");

static void glfw_error_callback(int error, const char* description)
{
    LOGGER_ERROR("GLFW error {}: {}", error, description);
}

Application::Application(const ApplicationSpecification& spec) : spec(spec)
{
    // TODO: make generic
    const auto result = glfwInit();
    PORTAL_ASSERT(result == GLFW_TRUE, "Failed to initialize GLFW");
    glfwSetErrorCallback(glfw_error_callback);

    const WindowSpecification window_spec{
        .title = spec.name,
        .width = spec.width,
        .height = spec.height,
    };

    vulkan_context = std::make_unique<renderer::vulkan::VulkanContext>();
    window = make_reference<renderer::vulkan::VulkanWindow>(*vulkan_context, window_spec);
    window->set_event_callback(
        [this](auto& event)
        {
            on_event(event);
        }
        );

    renderer = make_reference<Renderer>(*vulkan_context, std::dynamic_pointer_cast<renderer::vulkan::VulkanWindow>(window).get());


    scheduler = std::make_unique<jobs::Scheduler>(spec.scheduler_worker_num);
    reference_manager = std::make_unique<ReferenceManager>();
    resource_database = std::make_unique<FolderResourceDatabase>(spec.resources_path);
    resource_registry = std::make_unique<ResourceRegistry>(*reference_manager, *resource_database, *scheduler, renderer->get_renderer_context());

    engine_context = make_reference<EngineContext>(
        *renderer,
        *resource_registry,
        *window
        );

    // TODO: remove this
    imgui_module = std::make_unique<ImGuiModule>(engine_context);
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
        engine_context->get_resource_registry().immediate_load<Scene>(STRING_ID("game/ABeautifulGame"));
        auto scene = engine_context->get_resource_registry().get<Scene>(STRING_ID("Scene0-Scene"));

        auto vulkan_window = std::dynamic_pointer_cast<renderer::vulkan::VulkanWindow>(window);

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
                vulkan_window->get_swapchain().begin_frame();

                // Resets descriptor pools
                renderer->clean_frame();
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
                            if (node->children.empty())
                            {
                                flags |= ImGuiTreeNodeFlags_Leaf;
                            }

                            ImGui::PushID(node_id++);

                            const bool is_mesh = reference_cast<scene::MeshNode>(node) != nullptr;
                            if (is_mesh)
                            {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
                            }

                            const bool open = ImGui::TreeNodeEx(node->id.string.data(), flags);

                            if (is_mesh)
                            {
                                ImGui::PopStyleColor();
                            }

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                const auto& translate = glm::vec3(node->local_transform[3]);
                                ImGui::Text("Position: %.2f, %.2f, %.2f", translate.x, translate.y, translate.z);
                                ImGui::EndTooltip();
                            }

                            if (open)
                            {
                                for (const auto& child : node->children)
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
                window->swap_buffers();


                current_frame_count = (current_frame_count + 1) % vulkan_window->get_swapchain().get_frames_in_flight();
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
    portal::EventDispatcher dispatcher(event);
    dispatcher.dispatch<WindowCloseEvent>(
        [&](WindowCloseEvent&)
        {
            should_stop.test_and_set();
            return false;
        }
        );
    dispatcher.dispatch<WindowResizeEvent>([&](WindowResizeEvent& e) { return on_resize(e); });
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

bool Application::on_resize(portal::WindowResizeEvent& e)
{
    if (e.get_width() == 0 || e.get_height() == 0)
        return false;

    const auto vulkan_window = std::dynamic_pointer_cast<renderer::vulkan::VulkanWindow>(window);
    vulkan_window->get_swapchain().on_resize(e.get_width(), e.get_height());
    renderer->on_resize(e.get_width(), e.get_height());
    return false;
}

} // portal
