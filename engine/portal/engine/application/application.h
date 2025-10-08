//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include "portal/engine/engine_context.h"
#include "portal/engine/events/event.h"
#include "portal/engine/events/window_events.h"
#include "portal/engine/imgui/im_gui_module.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/strings/string_id.h"

namespace portal
{
class Module;

struct ApplicationSpecification
{
    StringId name = STRING_ID("Portal Engine");
    size_t width = 1600;
    size_t height = 900;

    uint64_t sleep_duration = 0;

    bool resizeable = true;
};

class Application
{
public:
    explicit Application(const ApplicationSpecification& spec);
    ~Application();

    void run();
    void stop();

    void on_init();
    void on_shutdown();
    void on_update(float delta_time);

    void on_event(Event& event);

    void add_module(std::shared_ptr<Module> module);
    void remove_module(std::shared_ptr<Module> module);

    void render_gui();

private:
    bool on_resize(portal::WindowResizeEvent& e);

private:
    ApplicationSpecification spec;

    Ref<renderer::vulkan::VulkanContext> vulkan_context;

    std::shared_ptr<Window> window = nullptr;
    std::shared_ptr<Renderer> renderer = nullptr;
    std::shared_ptr<ResourceRegistry> resource_registry = nullptr;

    std::shared_ptr<EngineContext> engine_context;

    std::unique_ptr<ImGuiModule> imgui_module;

    size_t current_frame_count = 0;
    float last_frame_time = 0;
    float frame_time = 0;
    float time_step = 0;

    std::atomic_flag should_stop;
};

} // portal