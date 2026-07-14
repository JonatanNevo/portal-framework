//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "application.h"

#include <GLFW/glfw3.h>

#include "settings.h"
#include "portal/core/log.h"
#include "portal/core/timer.h"
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
    auto& settings = get_settings();
    const auto frames_in_flight = settings.get_setting<size_t>("application.frames_in_flight", 3);
    const auto denominator = settings.get_setting<size_t>("application.fixed_timestep_denominator", 60);
    const auto fixed_timestep = 1.f / denominator;

    Timer application_timer;
    try
    {
        should_stop.clear();
        // TODO: Improve the stats system, accumulate more stats, etc...
        FrameStats global_stats{};

        LOGGER_INFO("Starting application");
        prepare();

        application_timer.start();
        while (!should_stop.test())
        {
            process_events();
            engine_event_dispatcher.update();

            {
                FrameContext context{
                    .frame_index = current_frame,
                    .absolute_frame_index = absolute_frame,
                    .delta_time = time_step,
                    .fixed_delta_time = fixed_timestep,
                    .stats = global_stats
                };

                modules.begin_frame(context);
                {
                    auto update_start = std::chrono::high_resolution_clock::now();

                    input_event_dispatcher.update();

                    /// Update scene, physics, input, ...
                    // Fixed update
                    accumulator += time_step;
                    while (accumulator >= fixed_timestep) {
                        modules.fixed_update(context);
                        accumulator -= fixed_timestep;
                    }
                    context.interpolation_alpha = accumulator / fixed_timestep;

                    // Per frame update
                    modules.update(context);

                    const auto update_end = std::chrono::high_resolution_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(update_end - update_start);
                    context.stats.scene_update_time = elapsed.count() / 1000.f;

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
            absolute_frame++;

            frame_time = application_timer.tick<Timer::Seconds>();
            time_step = glm::min<float>(frame_time, 0.0333f);

            // Seconds to ms
            global_stats.frame_time = frame_time * 1000.f;
        }

        LOGGER_INFO("Application stopped");
    }
    catch (const std::exception& e)
    {
        LOG_FATAL("Exception in application loop: {}", e.what());
        throw;
    }
    catch (...)
    {
        LOG_FATAL("Fatal unknown exception in application loop");
        throw;
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
