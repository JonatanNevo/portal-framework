//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "modules/system_orchestrator.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/resources/resource_registry.h"
#include "window/window.h"

namespace portal
{
class EngineContext
{
public:
    EngineContext(
        Renderer& renderer,
        ResourceRegistry& registry,
        Window& window,
        InputManager& input,
        SystemOrchestrator& system_orchestrator
    )
        : renderer(renderer),
          resource_registry(registry),
          window(window),
          input(input),
          system_orchestrator(system_orchestrator)
    {}

    [[nodiscard]] const Renderer& get_renderer() const { return renderer.get(); }
    [[nodiscard]] Renderer& get_renderer() { return renderer.get(); }

    [[nodiscard]] const ResourceRegistry& get_resource_registry() const { return resource_registry.get(); }
    [[nodiscard]] ResourceRegistry& get_resource_registry() { return resource_registry.get(); }

    [[nodiscard]] const Window& get_window() const { return window.get(); }
    [[nodiscard]] Window& get_window() { return window.get(); }

    [[nodiscard]] const InputManager& get_input() const { return input.get(); }
    [[nodiscard]] InputManager& get_input() { return input.get(); }

    [[nodiscard]] const SystemOrchestrator& get_system_orchestrator() const { return system_orchestrator.get(); }
    [[nodiscard]] SystemOrchestrator& get_system_orchestrator() { return system_orchestrator.get(); }

protected:
    std::reference_wrapper<Renderer> renderer;
    std::reference_wrapper<ResourceRegistry> resource_registry;
    std::reference_wrapper<Window> window;
    std::reference_wrapper<InputManager> input;
    std::reference_wrapper<SystemOrchestrator> system_orchestrator;
};
}
