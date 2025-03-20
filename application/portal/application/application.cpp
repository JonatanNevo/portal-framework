//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "application.h"

#include "portal/application/module/module.h"

namespace portal
{
Application::Application() {}

void Application::add_module(std::shared_ptr<Module> module)
{
    PORTAL_CORE_ASSERT(module != nullptr, "Module must not be null");
    const auto module_hooks = module->get_hooks();
    modules.emplace_back(module);
    for (const auto& hook : module_hooks)
    {
        if (!hooks.contains(hook))
            hooks.emplace(hook, std::vector<Module*>{});

        hooks[hook].emplace_back(module.get());
    }
}

bool Application::prepare(const Configuration& config)
{
    const auto window_ptr = config.get_with_default<Window*>("window", nullptr);
    PORTAL_CORE_ASSERT(window_ptr != nullptr, "Window must not be null");

    auto& debug_info = get_debug_info();
    debug_info.insert<debug::fields::MinMax, float>("fps", fps);
    debug_info.insert<debug::fields::MinMax, float>("frame_time", frame_time);

    lock_simulation_speed = config.get_with_default("lock_simulation_speed", false);
    window = window_ptr;

    for (const auto& module : hooks[Module::Hook::OnAppStart])
    {
        module->on_start(config, debug_info);
    }

    return window_ptr != nullptr;
}

void Application::update(const float delta_time)
{
    fps = 1.0f / delta_time;
    frame_time = delta_time * 1000.0f;

    for (const auto& module : hooks[Module::Hook::OnUpdate])
    {
        module->on_update(delta_time);
    }
}

void Application::finish()
{
    for (const auto& module : hooks[Module::Hook::OnAppClose])
    {
        module->on_close();
    }
}

void Application::resize(const uint32_t width, const uint32_t height)
{
    for (const auto& module : hooks[Module::Hook::OnResize])
    {
        module->on_resize(width, height);
    }
}

void Application::input_event(const InputEvent& input_event)
{
}

void Application::on_error()
{
    for (const auto& module : hooks[Module::Hook::OnAppError])
    {
        module->on_error();
    }
}
}
