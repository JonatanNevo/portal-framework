//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "application.h"

#include <GLFW/glfw3.h>

#include "portal/core/log.h"
#include "portal/core/reference.h"
#include "portal/core/debug/assert.h"
#include "portal/engine/application/window.h"
#include "portal/engine/imgui/im_gui_module.h"
#include "portal/engine/renderer/vulkan/vulkan_window.h"
#include "portal/engine/resources/database/folder_resource_database.h"

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
    Log::init({.default_log_level = Log::LogLevel::Trace});

    // TODO: make generic
    const auto result = glfwInit();
    PORTAL_ASSERT(result == GLFW_TRUE, "Failed to initialize GLFW");
    glfwSetErrorCallback(glfw_error_callback);

    const WindowSpecification window_spec{
        .title = spec.name,
        .width = spec.width,
        .height = spec.height,
    };

    vulkan_context = Ref<renderer::vulkan::VulkanContext>::create();
    window = std::make_shared<renderer::vulkan::VulkanWindow>(vulkan_context, window_spec);
    window->set_event_callback(
        [this](auto& event)
        {
            on_event(event);
        }
        );

    renderer = std::make_shared<Renderer>();
    renderer->init(vulkan_context, std::dynamic_pointer_cast<renderer::vulkan::VulkanWindow>(window).get());

    const auto resource_database = std::make_shared<portal::FolderResourceDatabase>(R"(C:\Code\portal-framework\engine\resources)");
    // resource_registry = std::make_shared<ResourceRegistry>();
    // resource_registry->initialize(renderer->get_gpu_context(), resource_database);

    engine_context = std::make_shared<EngineContext>(
        renderer.get(),
        resource_registry.get(),
        window.get()
        );

    PORTAL_ASSERT(engine_context->renderer != nullptr, "Null renderer");
    PORTAL_ASSERT(engine_context->window != nullptr, "Null window");
    PORTAL_ASSERT(engine_context->resource_registry != nullptr, "Null resource registry");

    // TODO: remove this
    imgui_module = std::make_unique<ImGuiModule>(engine_context);
}

Application::~Application()
{
    ref_utils::clean_all_references();
    glfwTerminate();

    Log::shutdown();
}

void Application::run()
{
    try
    {
        // TODO: Remove from here
        {
            engine_context->resource_registry->immediate_load<Scene>(STRING_ID("game/ABeautifulGame.gltf"));
            const auto scene = engine_context->resource_registry->get<Scene>(STRING_ID("Scene0-Scene"));
            engine_context->renderer->set_scene(scene);
        }

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
                    renderer->update_imgui(time_step);
                    renderer->update_scene(time_step);
                }

                {
                    // TODO: put in layer ?
                    renderer->draw_geometry();
                    imgui_module->end();
                }

                renderer->end_frame();
                window->swap_buffers();


                current_frame_count = (current_frame_count + 1) % vulkan_window->get_swapchain().get_frames_in_flight();
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
