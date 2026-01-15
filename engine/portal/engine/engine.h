//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <portal/application/application.h>

#include "ecs/registry.h"
#include "portal/application/settings.h"
#include "portal/engine/engine_context.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/window/window_event_consumer.h"
#include "resources/database/resource_database_facade.h"

namespace portal
{

/**
 * @brief Main engine application class.
 *
 * Engine initializes and owns the core subsystems: Vulkan context, window,
 * swapchain, and ECS registry. Supports both runtime and editor modes.
 */
class Engine : public Application, public WindowEventConsumer
{
public:
    /**
     * @brief Constructs the engine.
     * @param properties Application configuration.
     * @param editor If true, initializes in editor mode with EditorModule.
     */
    explicit Engine(const ApplicationProperties& properties, bool editor);

    ~Engine() override;

    /** @brief Initializes engine subsystems and modules. */
    void prepare() override;

    /** @brief Sets up a scene for rendering. */
    void setup_scene(ResourceReference<Scene> scene) const;

    /** @brief Processes window and input events. */
    void process_events() override;

    void on_resize(WindowExtent extent) override;
    void on_focus(bool) override {};
    void on_close() override;

    [[nodiscard]] EngineContext& get_engine_context() const { return *engine_context; }

private:
    ecs::Registry ecs_registry{};
    std::unique_ptr<renderer::vulkan::VulkanContext> vulkan_context = nullptr;
    Reference<Window> window = nullptr;
    Reference<renderer::vulkan::VulkanSwapchain> swapchain = nullptr;
    std::unique_ptr<EngineContext> engine_context = nullptr;
};

} // portal
