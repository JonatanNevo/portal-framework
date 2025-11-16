//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include "portal/engine/engine_context.h"
#include "portal/core/events/event.h"
#include "portal/engine/events/window_events.h"
#include "portal/engine/imgui/im_gui_module.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/core/strings/string_id.h"
#include "portal/engine/window/window_event_consumer.h"

namespace portal
{
class Module;

struct ApplicationSpecification
{
    StringId name = STRING_ID("Portal Engine");
    size_t width = 1600;
    size_t height = 900;

    bool resizeable = true;

    //TODO: move to settings
    std::filesystem::path resources_path;
    int32_t scheduler_worker_num = 1;
};

class Application: public WindowEventConsumer
{
public:
    explicit Application(const ApplicationSpecification& spec);
    ~Application() override;

    void run();
    void stop();

    void on_init();
    void on_shutdown();
    void on_update(float delta_time);

    void on_event(Event& event);

    void add_module(std::shared_ptr<Module> module);
    void remove_module(std::shared_ptr<Module> module);

    void render_gui();

    void on_resize(WindowExtent extent) override;
    void on_focus(bool set_focused) override;
    void on_close() override;


private:
    ApplicationSpecification spec;

    std::unique_ptr<Input> input = nullptr;
    std::unique_ptr<renderer::vulkan::VulkanContext> vulkan_context = nullptr;
    Reference<Window> window = nullptr;
    Reference<Renderer> renderer = nullptr;

    std::unique_ptr<jobs::Scheduler> scheduler = nullptr;
    std::unique_ptr<ReferenceManager> reference_manager = nullptr;
    std::unique_ptr<ResourceDatabase> resource_database = nullptr;
    std::unique_ptr<ResourceRegistry> resource_registry = nullptr;

    std::unique_ptr<ImGuiModule> imgui_module;

    Reference<EngineContext> engine_context;

    size_t current_frame_count = 0;
    float last_frame_time = 0;
    float frame_time = 0;
    float time_step = 0;
    [[maybe_unused]] bool focused = true;

    std::atomic_flag should_stop;

    llvm::SmallVector<std::reference_wrapper<EventHandler>> event_handlers;
};

} // portal