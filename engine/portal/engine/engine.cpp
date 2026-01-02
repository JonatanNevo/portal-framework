//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "engine.h"

#include "portal/application/settings.h"
#include "editor/editor_module.h"
#include "modules/system_orchestrator.h"
#include "portal/engine/imgui/imgui_module.h"
#include "portal/engine/modules/scheduler_module.h"
#include "portal/engine/resources/resources/composite.h"
#include "portal/engine/window/glfw_window.h"
#include "resources/database/resource_database_facade.h"
#include "portal/engine/resources/source/resource_source.h"

namespace portal
{
static auto logger = Log::get_logger("Engine");

std::unique_ptr<resources::ResourceSource> Engine::find_icon_source(Settings& settings, ResourceDatabaseFacade& resource_database)
{
    std::unique_ptr<resources::ResourceSource> icon_source;
    auto icon_id = STRING_ID(settings.get_setting<std::string>("application.icon").value_or("engine/portal_icon_64x64"));
    auto icon_result = resource_database.find(icon_id);
    if (icon_result.has_value())
        icon_source = resource_database.create_source(icon_id, icon_result.value());
    return icon_source;
}

Engine::Engine(const ApplicationProperties& properties) : Application(properties)
{
    auto& settings = Settings::get();
    auto& input = modules.add_module<InputManager>(
        [this](auto& event)
        {
            on_event(event);
        }
    );

    auto& resource_database = modules.add_module<ResourceDatabaseFacade>();
    if (settings.get_setting<bool>("engine.include_engine_resources").value_or(true))
    {
        resource_database.register_database({DatabaseType::Folder, "engine"});
    }

    std::unique_ptr<resources::ResourceSource> icon_source = find_icon_source(settings, resource_database);

    const WindowProperties window_properties{
        .title = properties.name,
        .extent = {properties.width, properties.height},
        .icon_source = icon_source.get(),
        .requested_frames_in_flight = properties.frames_in_flight,
    };
    window = make_reference<GlfwWindow>(window_properties, CallbackConsumers{*this, input});

    vulkan_context = renderer::vulkan::VulkanContext::create();
    auto& renderer = modules.add_module<Renderer>(*vulkan_context);

    modules.add_module<SchedulerModule>(settings.get_setting<int32_t>("application.scheduler-threads").value_or(0));
    modules.add_module<ReferenceManager>();
    auto& system_orchestrator = modules.add_module<SystemOrchestrator>();

    const auto descriptions = settings.get_setting<std::vector<DatabaseDescription>>("engine.resources");
    for (auto& description : descriptions.value_or(std::vector<DatabaseDescription>{}))
    {
        resource_database.register_database(description);
    }

    auto& registry = modules.add_module<ResourceRegistry>(renderer.get_renderer_context());

    auto surface = window->create_surface(*vulkan_context);
    // TODO: find better surface control
    vulkan_context->get_device().add_present_queue(*surface);

    auto swapchain = make_reference<renderer::vulkan::VulkanSwapchain>(*vulkan_context, surface);
    renderer.set_swapchain(swapchain);
    this->properties.frames_in_flight = swapchain->get_image_count();


    modules.add_module<ImGuiModule>(*window);
    modules.add_module<EditorModule>();

    // TODO: find a better way of subscribing to this
    event_handlers.emplace_back(*window);

    // TODO: make a O(1) lookup inside the module stack, will make this class redundant
    engine_context = std::make_unique<EngineContext>(
        renderer,
        registry,
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
    engine_context->get_renderer().cleanup();
    ecs_registry.clear();
    modules.clean();

    vulkan_context.reset();
    glfwTerminate();
}

void Engine::setup_scene(const ResourceReference<Scene>& scene) const
{
    engine_context->get_system_orchestrator().set_registry(scene->get_registry());
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
