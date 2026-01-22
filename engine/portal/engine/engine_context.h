//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "modules/resources_module.h"
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
        ecs::Registry& ecs_registry,
        ResourcesModule& resources_module,
        Window& window,
        InputManager& input,
        SystemOrchestrator& system_orchestrator
    )
        : ecs_registry(ecs_registry),
          resources_module(resources_module),
          window(window),
          input(input),
          system_orchestrator(system_orchestrator)
    {}

    [[nodiscard]] const ecs::Registry& get_ecs_registry() const { return ecs_registry.get(); }
    [[nodiscard]] ecs::Registry& get_ecs_registry() { return ecs_registry.get(); }

    [[nodiscard]] const ResourceRegistry& get_resource_registry() const { return resources_module.get().get_registry(); }
    [[nodiscard]] ResourceRegistry& get_resource_registry() { return resources_module.get().get_registry(); }

    [[nodiscard]] const ResourcesModule& get_resources() const { return resources_module.get(); }
    [[nodiscard]] ResourcesModule& get_resources() { return resources_module.get(); }

    [[nodiscard]] const Window& get_window() const { return window.get(); }
    [[nodiscard]] Window& get_window() { return window.get(); }

    [[nodiscard]] const InputManager& get_input() const { return input.get(); }
    [[nodiscard]] InputManager& get_input() { return input.get(); }

    [[nodiscard]] const SystemOrchestrator& get_system_orchestrator() const { return system_orchestrator.get(); }
    [[nodiscard]] SystemOrchestrator& get_system_orchestrator() { return system_orchestrator.get(); }

protected:
    std::reference_wrapper<ecs::Registry> ecs_registry;
    std::reference_wrapper<ResourcesModule> resources_module;
    std::reference_wrapper<Window> window;
    std::reference_wrapper<InputManager> input;
    std::reference_wrapper<SystemOrchestrator> system_orchestrator;
};
}
