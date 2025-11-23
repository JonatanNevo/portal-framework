//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "application.h"

#include <GLFW/glfw3.h>

#include "portal/core/log.h"
#include "portal/core/debug/profile.h"

namespace portal
{
class WindowResizeEvent;
class WindowCloseEvent;

static auto logger = Log::get_logger("Application");

Application::Application(const ApplicationProperties& properties) : properties(properties)
{}

Application::~Application()
{
    modules.clean();
}

void Application::run()
{
    try
    {
        should_stop.clear();

        LOGGER_INFO("Starting application");
        while (!should_stop.test())
        {
            process_events();

            {
                // TODO: this allocates a new vector for each frame, use some cache instead
                renderer::FrameContext context{
                    .frame_index = current_frame_count,
                    .delta_time = time_step,
                    .depth_image = depth_image->get_image_info().image.get_handle(),
                    .depth_image_view = depth_image->get_image_info().view,
                    .command_buffer = frames[current_frame].command_buffer,
                    .resources = frames[current_frame],
                    .stats = {
                        .frame_time = last_frame_time
                    }
                };

                modules.begin_frame(context);

                {
                    // Update scene, physics, input, ...
                    modules.update(context);
                    modules.gui_update(context);

                    // Draw geometry
                    modules.post_update(context);
                }

                modules.end_frame(context);

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

void Application::on_event(Event& event)
{
    for (auto& handler : event_handlers)
        handler.get().on_event(event);
}

bool Application::should_run() const
{
    return !should_stop.test();
}
} // portal
