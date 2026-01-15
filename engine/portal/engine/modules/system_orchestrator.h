//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>

#include "scheduler_module.h"
#include "portal/application/modules/module.h"
#include "portal/input/input_manager.h"
#include "portal/engine/ecs/registry.h"

namespace portal
{
class BasePlayerInputSystem;
class BaseCameraSystem;
class TransformHierarchySystem;
class SceneRenderingSystem;

// TODO: support dependencies between systems and parallel system execution
// TODO: support dynamic system declaration
// TODO: support creating system stack from file
class SystemOrchestrator final : public TaggedModule<Tag<ModuleTags::Update, ModuleTags::FrameLifecycle>, SchedulerModule, InputManager>
{
public:
    explicit SystemOrchestrator(ModuleStack& stack);
    ~SystemOrchestrator() override;

    void set_active_scene(Scene& scene);
    [[nodiscard]] Scene* get_active_scene() const { return active_scene; }

    void register_systems(ecs::Registry& registry);

    void begin_frame(FrameContext& frame) override;
    void update(FrameContext& frame) override;

private:
    // TODO: support multiple registries in parallel?
    ecs::Registry* active_registry = nullptr;
    Scene* active_scene = nullptr;

    std::unique_ptr<BasePlayerInputSystem> player_input_system;
    std::unique_ptr<BaseCameraSystem> camera_system;
    std::unique_ptr<TransformHierarchySystem> transform_system;
    std::unique_ptr<SceneRenderingSystem> scene_rendering_system;
};
} // portal
