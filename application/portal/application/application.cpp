//
// Created by thejo on 5/23/2025.
//

#include "portal/application/application.h"
#include "portal/application/window/glfw_window.h"

namespace portal
{
Application::Application(const ApplicationSettings& specs): settings(specs)
{
    init();
}

Application::~Application()
{
    shutdown();
}

void Application::run()
{
    while (running)
    {
        for (const auto& layer: layer_stack)
        {
            const bool do_update = layer->pre_update(time_step);
            if (do_update)
                layer->update(time_step);
        }

        for (const auto& layer: layer_stack)
            layer->post_update(time_step);

        if (settings.main_loop_sleep_duration > 0.f)
            std::this_thread::sleep_for( std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<float>(settings.main_loop_sleep_duration)));

        const float time = get_time();
        frame_time = time - last_frame_time;
        time_step = glm::min<float>(frame_time, 0.0333f);
        last_frame_time = time;
    }
}

void Application::close()
{
    running = false;
}

void Application::init()
{
    // Initialize logging
    Log::init();

    // TODO: have a generic way for passing window class
    auto&& glfw_window = std::make_shared<GLFWWindow>();
    context.window = std::dynamic_pointer_cast<Window>(glfw_window);
    context.window->initialize(settings.window_settings);

    context.render_target = std::dynamic_pointer_cast<RenderTarget>(glfw_window);

    // TODO: This can cause multiple calls for `on_context_change` for each layer before we even started, maybe change layer lifecycle
    for (const auto& layer: layer_stack)
        layer->on_context_change(&context);

    // Set get_time callback based on requested method
    if (settings.platform_timer)
    {
        get_time = [this]() { return context.window->get_time(); };
    }
    else
    {
        timer.start();
        get_time = [this]() { return timer.elapsed(); };
    }
    running = true;
}

void Application::shutdown()
{
    running = false;

    for (const auto& layer : layer_stack)
        layer->on_detach();
    layer_stack.clear();

    if (context.window)
        context.window->shutdown();
    context.window = nullptr;

    Log::shutdown();
}
}
