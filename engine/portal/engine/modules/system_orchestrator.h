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
#include "portal/engine/systems/base_camera_system.h"
#include "portal/engine/systems/base_player_input_system.h"
#include "portal/engine/systems/scene_rendering_system.h"
#include "portal/engine/systems/transform_hierarchy_system.h"

namespace portal
{

// TODO: support dependencies between systems and parallel system execution
// TODO: support dynamic system declaration
// TODO: support creating system stack from file
class SystemOrchestrator final : public TaggedModule<Tag<ModuleTags::Update, ModuleTags::FrameLifecycle>, ecs::Registry, SchedulerModule, InputManager>
{
public:
    explicit SystemOrchestrator(ModuleStack& stack);
    void set_active_scene(Scene& scene);
    [[nodiscard]] Scene* get_active_scene() const { return active_scene; }

    void begin_frame(FrameContext& frame) override;
    void update(FrameContext& frame) override;

private:
    Scene* active_scene = nullptr;

    std::unique_ptr<BasePlayerInputSystem> player_input_system;
    std::unique_ptr<BaseCameraSystem> camera_system;
    std::unique_ptr<TransformHierarchySystem> transform_system;
    std::unique_ptr<SceneRenderingSystem> scene_rendering_system;
};
} // portal
