//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "engine.h"

#include "portal/engine/imgui/imgui_module.h"
#include "portal/engine/modules/scheduler_module.h"
#include "portal/engine/resources/database/folder_resource_database.h"
#include "portal/engine/resources/resources/composite.h"
#include "portal/engine/window/glfw_window.h"

namespace portal
{
static auto logger = Log::get_logger("Engine");

Engine::Engine(const ApplicationProperties& properties) : Application(properties)
{
    auto& input = modules.add_module<Input>(
        [this](auto& event)
        {
            on_event(event);
        }
    );

    const WindowProperties window_properties{
        .title = properties.name,
        .extent = {properties.width, properties.width},
    };
    window = make_reference<GlfwWindow>(window_properties, CallbackConsumers{*this, input});

    vulkan_context = std::make_unique<renderer::vulkan::VulkanContext>();

    auto surface = window->create_surface(*vulkan_context);
    // TODO: find better surface control
    vulkan_context->get_device().add_present_queue(*surface);

    auto swapchain = make_reference<renderer::vulkan::VulkanSwapchain>(*vulkan_context, surface);
    auto& renderer = modules.add_module<Renderer>(*vulkan_context, swapchain);

    modules.add_module<SchedulerModule>(properties.scheduler_worker_num);
    modules.add_module<ReferenceManager>();
    modules.add_module<FolderResourceDatabase>(properties.resources_path);
    auto& registry = modules.add_module<ResourceRegistry>(renderer.get_renderer_context());

    modules.add_module<ImGuiModule>(*window);

    // TODO: find a better way of subscribing to this
    event_handlers.emplace_back(*window);
    event_handlers.emplace_back(renderer);

    // TODO: make a O(1) lookup inside the module stack, will make this class redundant
    engine_context = std::make_unique<EngineContext>(
        renderer,
        registry,
        *window,
        input
    );

    modules.build_dependency_graph();
}

Engine::~Engine()
{
    LOGGER_INFO("Shutting down Engine");
    vulkan_context->get_device().wait_idle();
    modules.clean();

    vulkan_context.release();
    glfwTerminate();
}

void Engine::setup_scene() const
{
    // TODO: Remove from here
    [[maybe_unused]] auto composite = engine_context->get_resource_registry().immediate_load<Composite>(STRING_ID("game/ABeautifulGame"));
    auto scene = engine_context->get_resource_registry().get<Scene>(STRING_ID("game/gltf-Scene-Scene"));
    PORTAL_ASSERT(scene.get_state() == ResourceState::Loaded, "Failed to load scene");
}

void Engine::process_events()
{
    window->process_events();
}

void Engine::on_resize(const WindowExtent extent)
{
    if (extent.width == 0 || extent.height == 0)
        return;

    const auto glfw_window = reference_cast<GlfwWindow>(window);
    auto [width, height] = glfw_window->resize(extent);

    engine_context->get_renderer().on_resize(width, height);
}

void Engine::on_close()
{
    should_stop.test_and_set();
}
} // portal
