//
// Created by Jonatan Nevo on 31/01/2025.
//

#include "headless_application.h"

#include <utility>

#include <portal/core/log.h>

namespace portal
{
static HeadlessApplication* s_instance = nullptr;

HeadlessApplication::HeadlessApplication(ApplicationSpecs specs) : specs(std::move(specs))
{
    s_instance = this;
    init();
}

HeadlessApplication::~HeadlessApplication()
{
    shutdown();
    s_instance = nullptr;
}

HeadlessApplication& HeadlessApplication::get() { return *s_instance; }

void HeadlessApplication::init()
{
    Log::init();
    app_timer.start();
    running = true;
}

void HeadlessApplication::shutdown()
{
    running = false;

    for (const auto& layer : layer_stack)
        layer->on_detach();
    layer_stack.clear();

    Log::shutdown();
}

void HeadlessApplication::run()
{
    while (running)
    {
        for (const auto& layer : layer_stack)
            layer->on_update(time_step);

        if (specs.sleep_duration > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(specs.sleep_duration));

        const float time = get_time();
        frame_time = time - last_frame_time;
        time_step = glm::min<float>(frame_time, 0.0333f);
        last_frame_time = time;
    }
}

void HeadlessApplication::close() { running = false; }

float HeadlessApplication::get_time() { return app_timer.elapsed(); }
} // namespace portal
