//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "application.h"

#include <GLFW/glfw3.h>

#include "settings.h"
#include "portal/core/log.h"
#include "portal/core/debug/profile.h"

namespace portal
{
class Project;

static auto logger = Log::get_logger("Application");

Application::Application(const ApplicationProperties& properties) : properties(properties)
{}

Application::~Application()
{
    modules.clean();
}

void Application::build_dependency_graph()
{
    modules.build_dependency_graph();
}

void Application::run()
{
    auto frames_in_flight =  get_settings().get_setting<size_t>("application.frames_in_flight", 3);
    try
    {
        should_stop.clear();
        // TODO: Improve the stats system, accumulate more stats, etc...
        FrameStats global_stats{};

        LOGGER_INFO("Starting application");
        prepare();

        while (!should_stop.test())
        {
            process_events();
            engine_event_dispatcher.update();

            {
                FrameContext context{
                    .frame_index = current_frame,
                    .delta_time = time_step,
                    .stats = global_stats
                };

                modules.begin_frame(context);
                {
                    input_event_dispatcher.update();
                    // Update scene, physics, input, ...
                    modules.update(context);

                    // TODO: will this differ between runtime and editor?
                    // Draw gui
                    modules.gui_update(context);

                    // Draw geometry
                    modules.post_update(context);
                }
                modules.end_frame(context);

                PORTAL_FRAME_MARK();

                global_stats = context.stats;
            }

            current_frame = (current_frame + 1) % frames_in_flight;
            // TODO: in headless application I wont have `glfwGetTime` use counter instead?
            const auto time = static_cast<float>(glfwGetTime());
            frame_time = time - last_frame_time;
            time_step = glm::min<float>(frame_time, 0.0333f);
            last_frame_time = time;

            // Seconds to ms
            global_stats.frame_time = frame_time * 1000.f;
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

bool Application::should_run() const
{
    return !should_stop.test();
}
} // portal
