//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <nlohmann/json.hpp>

#include "portal/core/log.h"
#include "portal/engine/application/window.h"
#include "portal/engine/events/window_events.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/vulkan/vulkan_window.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/folder_resource_database.h"

using namespace portal;

int main()
{
    portal::Log::init({.default_log_level = portal::Log::LogLevel::Trace});
    try
    {
        auto context = portal::Ref<portal::renderer::vulkan::VulkanContext>::create();

        const portal::WindowSpecification window_spec;
        portal::renderer::vulkan::VulkanWindow window(context, window_spec);

        portal::Renderer renderer;

        context->init();
        window.init();
        renderer.init(context, &window);

        auto resource_database = std::make_shared<portal::FolderResourceDatabase>("C:/Code/portal-framework/engine/resources");
        auto registry = ResourceRegistry();
        registry.initialize(renderer.get_gpu_context(), resource_database);

        // auto shader = registry.immediate_load<portal::Shader>(STRING_ID("PBR/pbr.slang"));;
        {
            registry.immediate_load<portal::Scene>(STRING_ID("game/ABeautifulGame.gltf"));
            auto scene = registry.get<portal::Scene>(STRING_ID("Scene0-Scene"));
            renderer.set_scene(scene);
        }

        size_t current_frame_count = 0;
        float last_frame_time = 0;
        float frame_time = 0;
        float time_step = 0;
        bool running = true;

        auto on_resize = [&](portal::WindowResizeEvent& e)-> bool
        {
            if (e.get_width() == 0 || e.get_height() == 0)
                return false;

            window.get_swapchain().on_resize(e.get_width(), e.get_height());
            renderer.on_resize(e.get_width(), e.get_height());
            return false;
        };

        auto on_event = [&](portal::Event& event) -> void
        {
            portal::EventDispatcher dispatcher(event);
            dispatcher.dispatch<WindowCloseEvent>([&](WindowCloseEvent& e) { running = false; return false; });
            dispatcher.dispatch<WindowResizeEvent>([&](WindowResizeEvent& e) { return on_resize(e); } );
        };
        window.set_event_callback(on_event);

        // Main run loop
        {

            while (running) // TODO: use `close window callback` to signify closing glfwWindowShouldClose
            {
                // Process Events
                {
                    // Process input events
                    window.process_events();
                    // Run queues events
                }

                // Run single iteration of renderer::draw
                {
                    window.get_swapchain().begin_frame();

                    // Resets descriptor pools
                    renderer.clean_frame();
                    renderer.begin_frame();

                    {
                        // TODO: put in layer
                        renderer.update_imgui(time_step);
                        renderer.update_scene(time_step);
                    }

                    {
                        // TODO: put in layer ?
                        renderer.draw_geometry();
                        renderer.draw_imgui();
                    }

                    renderer.end_frame();
                    window.swap_buffers();


                    current_frame_count = (current_frame_count + 1) % window.get_swapchain().get_frames_in_flight();
                }

                float time = glfwGetTime();
                frame_time = time - last_frame_time;
                time_step = glm::min<float>(frame_time, 0.0333f);
                last_frame_time = time;
            }
        }

        renderer.cleanup();
        registry.shutdown();
        window.shutdown();
        context.reset();
    }
    catch (const std::exception& e)
    {
        LOG_FATAL("Exception caught: {}", e.what());
    }
    catch (...)
    {
        LOG_FATAL("Fatal unknown exception caught");
    }

    ref_utils::clean_all_references();
    portal::Log::shutdown();
}
