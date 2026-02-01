//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>
#include <entt/signal/dispatcher.hpp>

#include <llvm/ADT/SmallVector.h>
#include <portal/core/strings/string_id.h>

#include "settings.h"
#include "modules/module_stack.h"

namespace portal
{
class Project;
/**
 * @struct ApplicationProperties application.h portal/application/application.h
 * Configuration properties for Portal application initialization.
 *
 * ApplicationProperties contains startup configuration passed to the Application
 * constructor, including window dimensions, frame buffering settings, resource
 * paths, and scheduler configuration.
 */
struct ApplicationProperties
{
    StringId name = STRING_ID("Portal Engine");
    size_t width = 1600;
    size_t height = 900;

    bool resizeable = true;
};

/**
 * @class Application application.h
 * The main application class providing the game loop and module orchestration.
 *
 * Application is the entry point for Portal Framework applications. It manages the
 * main game loop, coordinates module lifecycle execution, and handles frame timing.
 *
 * The typical flow:
 * 1. Construct Application (or derived class like Engine) with ApplicationProperties
 * 2. Register modules with modules.add_module<T>()
 * 3. Call modules.build_dependency_graph()
 * 4. Call run() to start the game loop
 *
 * Each frame executes:
 * - process_events() - virtual method for windowing/input (override in derived class)
 * - modules.begin_frame(context)
 * - modules.update(context)
 * - modules.gui_update(context)
 * - modules.post_update(context)
 * - modules.end_frame(context)
 *
 * Derived classes (typically Engine) register their specific modules in the constructor
 * and override process_events() for platform-specific event handling.
 *
 * Example:
 * @code
 * int main() {
 *     ApplicationProperties props{
 *         .name = STRING_ID("My Game"),
 *         .width = 1920,
 *         .height = 1080
 *     };
 *     Engine engine(props);  // Engine derives from Application
 *     return engine.run();
 * }
 * @endcode
 */
class Application
{
public:
    /**
     * Construct application with configuration properties.
     * @param properties Configuration for window, buffering, resources, etc.
     */
    explicit Application(const ApplicationProperties& properties);
    virtual ~Application();

    virtual void build_dependency_graph();

    virtual void prepare() {};

    /**
     * Start the main game loop.
     *
     * Executes the frame loop until should_run() returns false. Each iteration
     * processes events, executes module lifecycle hooks in sequence, and updates
     * frame timing. Blocks until the application stops.
     */
    void run();

    /**
     * Request the application to stop.
     * Sets the should_stop flag, causing the game loop to exit after the current frame.
     */
    void stop();

    /**
     * Process platform events (override in derived classes).
     * Called at the beginning of each frame before module lifecycle execution.
     */
    virtual void process_events() {};

    /**
     * Check if the application should continue running.
     * @return true if the game loop should continue, false to exit
     */
    [[nodiscard]] virtual bool should_run() const;

protected:
    virtual ProjectSettings& get_settings() const = 0;

protected:
    ApplicationProperties properties;
    ModuleStack modules;

    size_t current_frame = 0;
    float last_frame_time = 0;
    float frame_time = 0;
    float time_step = 0;

    std::atomic_flag should_stop;
    entt::dispatcher engine_event_dispatcher;
    entt::dispatcher input_event_dispatcher;
};

/**
 * Factory function for creating the application instance.
 *
 * User implements this function to instantiate their Application-derived class.
 * Called by the framework's main entry point.
 *
 * @param arc Argument count from main()
 * @param argv Argument vector from main()
 * @return Unique pointer to the application instance
 */
std::unique_ptr<Application> create_application(int arc, char** argv);
} // portal
