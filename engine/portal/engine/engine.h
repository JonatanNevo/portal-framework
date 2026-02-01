//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/application/application.h>

#include "portal/application/settings.h"
#include "portal/engine/engine_context.h"
#include "portal/engine/renderer/renderer.h"
#include "window/window_events.h"

namespace portal
{

/**
 * @brief Main engine application class.
 *
 * Engine initializes and owns the core subsystems: Vulkan context, window,
 * swapchain, and ECS registry. Supports both runtime and editor modes.
 */
class Engine : public Application
{
public:
    /**
     * @brief Constructs the engine.
     * @param project
     * @param properties Application configuration.
     */
    explicit Engine(const Reference<Project>& project, const ApplicationProperties& properties);

    ~Engine() override;

    /** @brief Initializes engine subsystems and modules. */
    void prepare() override;

    /** @brief Processes window and input events. */
    void process_events() override;

    [[nodiscard]] EngineContext& get_engine_context() const { return *engine_context; }

protected:
    void on_resize(WindowResizeEvent event) const;
    void on_close();

    ProjectSettings& get_settings() const override;

private:
    Reference<Project> project;
    Reference<Window> window = nullptr;

    std::unique_ptr<renderer::vulkan::VulkanContext> vulkan_context = nullptr;
    Reference<renderer::vulkan::VulkanSwapchain> swapchain = nullptr;
    std::unique_ptr<EngineContext> engine_context = nullptr;
};

} // portal
