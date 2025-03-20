//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "platform.h"

#include <iostream>

namespace portal
{
const uint32_t Platform::MIN_WINDOW_WIDTH  = 420;
const uint32_t Platform::MIN_WINDOW_HEIGHT = 320;

ExitCode Platform::initialize(const std::function<std::unique_ptr<Application>()> app_factory)
{
    Log::init();
    LOG_CORE_INFO_TAG("Platform", "Logger initialized");

    create_window(window_properties);
    if (!window)
    {
        LOG_CORE_ERROR_TAG("Platform", "Window creation failed");
        return ExitCode::FatalError;
    }

    // TODO: add more stuff to config
    auto config = Configuration();
    config.set<Window*>("window", window.get());

    active_app = app_factory();
    if (!active_app)
    {
        LOG_CORE_ERROR_TAG("Platform", "Application creation failed");
        return ExitCode::FatalError;
    }

    if (!active_app->prepare(config))
    {
        LOG_CORE_ERROR_TAG("Platform", "Application prepare failed");
        return ExitCode::FatalError;
    }

    return ExitCode::Success;
}


ExitCode Platform::main_loop_frame()
{
    try
    {
        update();

        if (active_app->should_close())
            active_app->finish();

        window->process_events();

        if (window->should_close() || close_requested)
            return ExitCode::Close;
    }
    catch (std::exception& e)
    {
        LOG_CORE_ERROR_TAG("Platform", "Error Message: {}", e.what());
        LOG_CORE_ERROR_TAG("Platform", "Failed when running application {}", active_app->get_name());
        active_app->on_error();
        set_last_error(e.what());
        return ExitCode::FatalError;
    }
    return ExitCode::Success;
}

ExitCode Platform::main_loop()
{
    auto exit_code = ExitCode::Success;
    while (exit_code == ExitCode::Success)
    {
        exit_code = main_loop_frame();
    }
    return exit_code;
}

void Platform::update()
{
    auto delta_time = static_cast<float>(timer.tick<Timer::Seconds>());
    if (focused || always_render)
    {
        if (fixed_simulation_fps)
            delta_time = simulation_frame_time;

        // active_app->update_overlay(delta_time, [=]() {on_update_ui_overlay(*active_app->get_drawer());});
        active_app->update(delta_time);

        // if (auto *app = dynamic_cast<VulkanSampleCpp *>(active_app.get()))
        // {
        //     if (app->has_render_context())
        //     {
        //         on_post_draw(reinterpret_cast<vkb::RenderContext &>(app->get_render_context()));
        //     }
        // }
        // else if (auto *app = dynamic_cast<VulkanSampleC *>(active_app.get()))
        // {
        //     if (app->has_render_context())
        //     {
        //         on_post_draw(app->get_render_context());
        //     }
        // }
    }
}

void Platform::terminate(ExitCode code)
{
    if (active_app)
        active_app->finish();

    active_app.reset();
    window.reset();

    Log::shutdown();

#ifdef PORTAL_PLATFORM_WINDOWS
    // Halt on all unsuccessful exit codes unless ForceClose is in use
    if (code != ExitCode::Success)
    {
        std::cout << "Press return to continue";
        std::cin.get();
    }
#endif
}

void Platform::close()
{
    if (window)
        window->close();

    // Fallback incase a window is not yet in use
    close_requested = true;
}

void Platform::force_simulation_fps(float fps)
{
    fixed_simulation_fps = true;
    simulation_frame_time = 1 / fps;
}


void Platform::force_render(const bool should_always_render)
{
    always_render = should_always_render;
}


void Platform::disable_input_processing()
{
    process_input_events = false;
}

void Platform::set_focus(const bool focused)
{
    this->focused = focused;
}

void Platform::set_window_properties(const Window::OptionalProperties& properties)
{
    window_properties.title = properties.title.has_value() ? properties.title.value() : window_properties.title;
    window_properties.mode = properties.mode.has_value() ? properties.mode.value() : window_properties.mode;
    window_properties.resizable = properties.resizable.has_value() ? properties.resizable.value() : window_properties.resizable;
    window_properties.vsync = properties.vsync.has_value() ? properties.vsync.value() : window_properties.vsync;
    window_properties.extent.width = properties.extent.width.has_value() ? properties.extent.width.value() : window_properties.extent.width;
    window_properties.extent.height = properties.extent.height.has_value() ? properties.extent.height.value() : window_properties.extent.height;
}

std::string& Platform::get_last_error()
{
    return last_error;
}

void Platform::set_last_error(const std::string& error)
{
    last_error = error;
}

Application& Platform::get_app()
{
    PORTAL_CORE_ASSERT(active_app, "Application is not valid");
    return *active_app;
}

Application& Platform::get_app() const
{
    PORTAL_CORE_ASSERT(active_app, "Application is not valid");
    return *active_app;
}

Window& Platform::get_window() const
{
    return *window;
}

void Platform::input_event(const InputEvent &input_event)
{
    if (process_input_events && active_app)
        active_app->input_event(input_event);
}

void Platform::resize(const uint32_t width, const uint32_t height)
{
    const auto extent = Window::Extent{std::max<uint32_t>(width, MIN_WINDOW_WIDTH), std::max<uint32_t>(height, MIN_WINDOW_HEIGHT)};
    if ((window) && (width > 0) && (height > 0))
    {
        auto [width, height] = window->resize(extent);

        if (active_app)
            active_app->resize(width, height);
    }
}

} // portal
