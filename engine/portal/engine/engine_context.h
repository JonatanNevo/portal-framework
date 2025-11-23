//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/renderer.h"
#include "portal/engine/resources/resource_registry.h"
#include "scene/scene_manager.h"
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
        Input& input,
        SceneManager& scene_manager
    )
        : renderer(renderer),
          resource_registry(registry),
          window(window),
          input(input),
          scene_manager(scene_manager)

    {}

    [[nodiscard]] const Renderer& get_renderer() const { return renderer.get(); }
    [[nodiscard]] Renderer& get_renderer() { return renderer.get(); }

    [[nodiscard]] const ResourceRegistry& get_resource_registry() const { return resource_registry.get(); }
    [[nodiscard]] ResourceRegistry& get_resource_registry() { return resource_registry.get(); }

    [[nodiscard]] const Window& get_window() const { return window.get(); }
    [[nodiscard]] Window& get_window() { return window.get(); }

    [[nodiscard]] const Input& get_input() const { return input.get(); }
    [[nodiscard]] Input& get_input() { return input.get(); }

    [[nodiscard]] const SceneManager& get_scene_manager() const { return scene_manager.get(); }
    [[nodiscard]] SceneManager& get_scene_manager() { return scene_manager.get(); }

protected:
    std::reference_wrapper<Renderer> renderer;
    std::reference_wrapper<ResourceRegistry> resource_registry;
    std::reference_wrapper<Window> window;
    std::reference_wrapper<Input> input;
    std::reference_wrapper<SceneManager> scene_manager;
};
}
