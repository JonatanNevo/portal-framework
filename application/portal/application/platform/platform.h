//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include "portal/application/application.h"
#include "portal/application/input_events.h"
#include "portal/application/window/window.h"
#include "portal/core/timer.h"

namespace portal
{
enum class ExitCode
{
    Success = 0, /* App executed as expected */
    Close,       /* App has been requested to close at initialization */
    FatalError   /* App encountered an unexpected error */
};

class Platform
{
public:
    virtual ~Platform() = default;


    /**
     * @brief Initialize the platform
     * @return An exit code representing the outcome of initialization
     */
    virtual ExitCode initialize(std::function<std::unique_ptr<Application>()> app_factory);

    /**
     * @brief Handles the main update and render loop
     * @return An exit code representing the outcome of the loop
     */
    ExitCode main_loop();

    /**
     * @brief Handles the update and render of a frame.
     * Called either from main_loop(), or from a platform-specific
     * frame-looping mechanism, typically tied to platform screen refeshes.
     * @return An exit code representing the outcome of the loop
     */
    ExitCode main_loop_frame();

    /**
     * @brief Runs the application for one frame
     */
    void update();

    /**
     * @brief Terminates the platform and the application
     * @param code Determines how the platform should exit
     */
    virtual void terminate(ExitCode code);

    /**
     * @brief Requests to close the platform at the next available point
     */
    virtual void close();

    std::string& get_last_error();
    void set_last_error(const std::string& error);

    virtual void resize(uint32_t width, uint32_t height);
    virtual void input_event(const InputEvent& input_event);
    Window& get_window() const;

    [[nodiscard]] Application& get_app() const;
    Application& get_app();

    void set_focus(bool focused);

    void force_simulation_fps(float fps);
    // Force the application to always render even if it is not in focus
    void force_render(bool should_always_render);
    void disable_input_processing();
    void set_window_properties(const Window::OptionalProperties& properties);

    // void on_post_draw(RenderContext &context);

    static const uint32_t MIN_WINDOW_WIDTH;
    static const uint32_t MIN_WINDOW_HEIGHT;

protected:
    /**
     * @brief Handles the creation of the window
     *
     * @param properties Preferred window configuration
     */
    virtual void create_window(const Window::Properties& properties) = 0;

    std::unique_ptr<Window> window = nullptr;
    std::unique_ptr<Application> active_app = nullptr;

    Window::Properties window_properties; /* Source of truth for window state */
    bool fixed_simulation_fps{false};     /* Delta time should be fixed with a fabricated value */
    bool always_render{false};            /* App should always render even if not in focus */
    float simulation_frame_time = 0.016f; /* A fabricated delta time */
    bool process_input_events{true};      /* App should continue processing input events */
    bool focused{true};                   /* App is currently in focus at an operating system level */
    bool close_requested{false};          /* Close requested */

private:
    Timer timer;
    std::string last_error;
};
} // portal
