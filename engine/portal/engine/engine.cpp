//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "engine.h"

#include "portal/application/settings.h"
#include "editor/editor_module.h"
#include "modules/resources_module.h"
#include "modules/system_orchestrator.h"
#include "portal/engine/modules/scheduler_module.h"
#include "portal/engine/resources/resources/composite.h"
#include "portal/engine/window/glfw_window.h"
#include "resources/database/resource_database_facade.h"

namespace portal
{
static auto logger = Log::get_logger("Engine");

Engine::Engine(const ApplicationProperties& properties, bool editor) : Application(properties)
{
    // Creating Input
    auto& settings = Settings::get();
    auto& input = modules.add_module<InputManager>(
        [this](auto& event)
        {
            on_event(event);
        }
    );

    modules.add_module<SchedulerModule>(settings.get_setting<int32_t>("application.scheduler-threads", 0));
    auto& system_orchestrator = modules.add_module<SystemOrchestrator>();

    // Creating vulkan context
    const WindowProperties window_properties{
        .title = properties.name,
        .extent = {properties.width, properties.height},
        .requested_frames_in_flight = settings.get_setting<size_t>("application.frames_in_flight", 3),
    };
    window = make_reference<GlfwWindow>(window_properties, CallbackConsumers{*this, input});
    // TODO: find a better way of subscribing to this
    event_handlers.emplace_back(*window);

    vulkan_context = renderer::vulkan::VulkanContext::create();

    auto& resources_module = modules.add_module<ResourcesModule>(*vulkan_context);

    auto surface = window->create_surface(*vulkan_context);
    // TODO: find better surface control
    vulkan_context->get_device().add_present_queue(*surface);
    swapchain = make_reference<renderer::vulkan::VulkanSwapchain>(*vulkan_context, surface);

    if (editor)
        modules.add_module<EditorModule>(*vulkan_context, *swapchain, *window);
    else
        modules.add_module<RuntimeModule>(*vulkan_context, *swapchain);

    // TODO: make a O(1) lookup inside the module stack, will make this class redundant
    engine_context = std::make_unique<EngineContext>(
        resources_module,
        *window,
        input,
        system_orchestrator
    );

    modules.build_dependency_graph();
}

Engine::~Engine()
{
    LOGGER_INFO("Shutting down Engine");
    vulkan_context->get_device().wait_idle();
    // engine_context->get_renderer().cleanup();
    ecs_registry.clear();
    modules.clean();
    swapchain.reset();

    vulkan_context.reset();
    glfwTerminate();
}

void Engine::prepare()
{
    const auto scene_id = Settings::get().get_setting<StringId>("engine.starting_scene");
    if (scene_id.has_value())
    {
        auto scene_reference = engine_context->get_resource_registry().immediate_load<Scene>(scene_id.value());
        scene_reference->set_viewport_bounds({0, 0, swapchain->get_width(), swapchain->get_height()});
        setup_scene(scene_reference);
    }
    else
    {
        // TODO: This will not be ordered, maybe use some default empty scene here instead
        // Take the first scene
        auto scene = engine_context->get_resource_registry().list_all_resources_of_type<Scene>() | std::ranges::views::take(1);
        scene.front()->set_viewport_bounds({0, 0, swapchain->get_width(), swapchain->get_height()});
        setup_scene(scene.front());
    }
}

void Engine::setup_scene(ResourceReference<Scene> scene) const
{
    engine_context->get_system_orchestrator().set_active_scene(*scene);
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

    swapchain->on_resize(width, height);
}

void Engine::on_close()
{
    should_stop.test_and_set();
}
} // portal
