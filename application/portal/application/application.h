//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once
#include <filesystem>
#include <string>

#include "portal/application/configuration.h"
#include "portal/application/input_events.h"
#include "portal/application/debug/debug_info.h"
#include "portal/application/module/module.h"
#include "portal/application/window/window.h"

namespace portal
{
namespace gui
{
    class Drawer;
}

class Module;

class Application
{
public:
    explicit Application();
    virtual ~Application() = default;

    virtual void add_module(std::shared_ptr<Module> module);

    /**
     * @brief Prepares the application for execution
     */
    virtual bool prepare(const Configuration& config);

    /**
     * @brief Updates the application
     * @param delta_time The time since the last update
     */
    virtual void update(float delta_time);

    /**
     * @brief Handles cleaning up the application
     */
    virtual void finish();

    /**
     * @brief Handles resizing of the window
     * @param width New width of the window
     * @param height New height of the window
     */
    virtual void resize(const uint32_t width, const uint32_t height);

    /**
     * @brief Handles input events of the window
     * @param input_event The input event object
     */
    virtual void input_event(const InputEvent& input_event);

    [[nodiscard]] const std::string& get_name() const { return name; }
    void set_name(const std::string& name) { this->name = name; }
    debug::DebugInfo& get_debug_info() { return debug_info; }

    template <class T>
    T* get_module() const;

    template <class T>
    bool using_module() const;

    [[nodiscard]] bool should_close() const { return requested_close; }

    // request the app to close
    // does not guarantee that the app will close immediately
    void close() { requested_close = true; }

    /**
     * @brief Calles when an application error occurs
     */
    void on_error();

protected:
    float fps{0.0f};
    float frame_time{0.0f}; // In ms
    uint32_t frame_count{0};
    uint32_t last_frame_count{0};
    bool lock_simulation_speed{false};
    Window* window{nullptr};

private:
    std::string name;

    std::vector<std::shared_ptr<Module>> modules;
    std::unordered_map<Module::Hook, std::vector<Module*>> hooks;

    debug::DebugInfo debug_info;
    bool requested_close{false};
};

template <class T>
bool Application::using_module() const
{
    return !modules::with_tags<T>(modules).empty();
}

template <class T>
T* Application::get_module() const
{
    PORTAL_CORE_ASSERT(using_module<T>(), "Module is not enabled but was requested");
    const auto modules = modules::with_tags<T>(modules);
    return dynamic_cast<T*>(modules[0]);
}
} // namespace portal
