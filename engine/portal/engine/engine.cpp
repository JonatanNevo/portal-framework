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
#include "project/project.h"
#include "resources/database/resource_database_facade.h"
#include "window/window_events.h"

namespace portal
{
static auto logger = Log::get_logger("Engine");

Engine::Engine(const Reference<Project>& project, const ApplicationProperties& properties) : Application(properties),
    project(project)
{
    engine_event_dispatcher.sink<WindowResizeEvent>().connect<&Engine::on_resize>(this);
    engine_event_dispatcher.sink<WindowClosedEvent>().connect<&Engine::on_close>(this);

    // Creating Input
    auto& settings = project->get_settings();
    auto& input = modules.add_module<InputManager>(engine_event_dispatcher, input_event_dispatcher);

    modules.add_module<SchedulerModule>(settings.get_setting<int32_t>("application.scheduler-threads", 0));
    auto& registry = modules.add_module<ecs::Registry>();
    auto& system_orchestrator = modules.add_module<SystemOrchestrator>();

    // Creating vulkan context
    const WindowProperties window_properties{
        .title = properties.name,
        .extent = {properties.width, properties.height},
        .decorated = project->get_type() != ProjectType::Editor,
        .requested_frames_in_flight = settings.get_setting<size_t>("application.frames_in_flight", 3),
    };
    window = make_reference<GlfwWindow>(project->get_settings(), window_properties, engine_event_dispatcher);

    vulkan_context = renderer::vulkan::VulkanContext::create();

    auto& resources_module = modules.add_module<ResourcesModule>(*project, *vulkan_context);

    auto surface = window->create_surface(*vulkan_context);
    // TODO: find better surface control
    vulkan_context->get_device().add_present_queue(*surface);
    swapchain = make_reference<renderer::vulkan::VulkanSwapchain>(project->get_settings(), *vulkan_context, surface);

    if (project->get_type() == ProjectType::Editor)
    {
        modules.add_module<EditorModule>(*project, *vulkan_context, *swapchain, *window, engine_event_dispatcher, input_event_dispatcher);
    }
    else
    {
        system_orchestrator.connect(input_event_dispatcher);
        modules.add_module<RuntimeModule>(*project, *vulkan_context, *swapchain, *window);
    }


    // TODO: make a O(1) lookup inside the module stack, will make this class redundant
    engine_context = std::make_unique<EngineContext>(
        registry,
        resources_module,
        *window,
        input,
        system_orchestrator
    );
}

Engine::~Engine()
{
    LOGGER_INFO("Shutting down Engine");
    vulkan_context->get_device().wait_idle();
    // engine_context->get_renderer().cleanup();

    engine_context->get_ecs_registry().clear();
    engine_context->get_system_orchestrator().clean();

    modules.clean();
    swapchain.reset();

    vulkan_context.reset();
    glfwTerminate();
}

void Engine::prepare()
{
    const auto scene_id = project->get_starting_scene();
    if (scene_id != INVALID_STRING_ID)
    {
        auto scene_reference = engine_context->get_resource_registry().immediate_load<Scene>(scene_id);
        scene_reference->set_viewport_bounds({0, 0, swapchain->get_width(), swapchain->get_height()});
        engine_context->get_system_orchestrator().set_active_scene(scene_reference);
    }
    else
    {
        // TODO: This will not be ordered, maybe use some default empty scene here instead
        // Take the first scene
        auto scene = engine_context->get_resource_registry().list_all_resources_of_type<Scene>() | std::ranges::views::take(1);
        scene.front()->set_viewport_bounds({0, 0, swapchain->get_width(), swapchain->get_height()});
        engine_context->get_system_orchestrator().set_active_scene(scene.front());
    }
}

void Engine::process_events()
{
    window->process_events();
}

void Engine::on_resize(const WindowResizeEvent event) const
{
    if (event.extent.width == 0 || event.extent.height == 0)
        return;

    const auto glfw_window = reference_cast<GlfwWindow>(window);
    auto [width, height] = glfw_window->resize(event.extent);

    swapchain->on_resize(width, height);
}

void Engine::on_close()
{
    should_stop.test_and_set();
}

ProjectSettings& Engine::get_settings() const
{
    return project->get_settings();
}
} // portal
