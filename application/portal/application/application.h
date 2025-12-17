//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include <llvm/ADT/SmallVector.h>
#include <portal/core/events/event.h>
#include <portal/core/strings/string_id.h>
#include <portal/core/events/event_handler.h>

#include "modules/module_stack.h"

namespace portal
{
struct ApplicationProperties
{
    StringId name = STRING_ID("Portal Engine");
    size_t width = 1600;
    size_t height = 900;

    bool resizeable = true;
    size_t frames_in_flight = 3;

    //TODO: move to settings
    std::filesystem::path resources_path;
    int32_t scheduler_worker_num = 1;
};

class Application
{
public:
    explicit Application(const ApplicationProperties& properties);
    virtual ~Application();

    void run();
    void stop();

    virtual void process_events() {};

    void on_event(Event& event);

    [[nodiscard]] virtual bool should_run() const;

protected:
    ApplicationProperties properties;
    ModuleStack modules;

    size_t current_frame = 0;
    float last_frame_time = 0;
    float frame_time = 0;
    float time_step = 0;

    std::atomic_flag should_stop;
    llvm::SmallVector<std::reference_wrapper<EventHandler>> event_handlers;
};

std::unique_ptr<Application> create_application(int arc, char** argv);
} // portal
