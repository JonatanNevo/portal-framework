//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "application.h"

#include <GLFW/glfw3.h>

#include "settings.h"
#include "portal/application/modules/base_module.h"
#include "portal/core/log.h"
#include "portal/core/debug/profile.h"

namespace portal
{
class WindowResizeEvent;
class WindowCloseEvent;

static auto logger = Log::get_logger("Application");

ApplicationProperties ApplicationProperties::from_settings()
{
    auto& settings = Settings::get();

    const auto width = settings.get_setting<size_t>("application.window.width");
    const auto height = settings.get_setting<size_t>("application.window.height");

    return ApplicationProperties{
        .name = STRING_ID(PORTAL_APPLICATION_NAME),
        .width = width.value(),
        .height = height.value()
    };
}

Application::Application(const ApplicationProperties& properties) : properties(properties)
{}

Application::~Application()
{
    modules.clean();
}

void Application::run()
{
    auto frames_in_flight = Settings::get().get_setting<size_t>("application.frames_in_flight", 3);
    try
    {
        should_stop.clear();

        // TODO: Improve the stats system, accumulate more stats, etc...
        FrameStats global_stats{};

        LOGGER_INFO("Starting application");
        while (!should_stop.test())
        {
            process_events();

            {
                FrameContext context{
                    .frame_index = current_frame,
                    .delta_time = time_step,
                    .stats = global_stats
                };

                modules.begin_frame(context);
                {
                    // Update scene, physics, input, ...
                    modules.update(context);

                    // Draw geometry
                    modules.post_update(context);

                    // TODO: will this differ between runtime and editor?
                    // Draw gui
                    modules.gui_update(context);
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
