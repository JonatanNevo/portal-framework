//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "system_orchestrator.h"

#include "portal/engine/scene/scene.h"
#include "portal/engine/systems/base_camera_system.h"
#include "portal/engine/systems/base_player_input_system.h"
#include "portal/engine/systems/scene_rendering_system.h"
#include "portal/engine/systems/transform_hierarchy_system.h"

namespace portal
{
SystemOrchestrator::SystemOrchestrator(ModuleStack& stack)
    : TaggedModule(stack, STRING_ID("System Orchestrator")),
      player_input_system(std::make_unique<BasePlayerInputSystem>(get_dependency<InputManager>())),
      camera_system(std::make_unique<BaseCameraSystem>()),
      transform_system(std::make_unique<TransformHierarchySystem>()),
      scene_rendering_system(std::make_unique<SceneRenderingSystem>())
{
    auto& registry = get_dependency<ecs::Registry>();

    player_input_system->register_to(registry);
    camera_system->register_to(registry);
    transform_system->register_to(registry);
    scene_rendering_system->register_to(registry);
}

void SystemOrchestrator::set_active_scene(Scene& scene)
{
    active_scene = &scene;
}

void SystemOrchestrator::begin_frame(FrameContext& frame)
{
    PORTAL_ASSERT(active_scene != nullptr, "Invalid scene, cannot run systems");
    frame.ecs_registry = &get_dependency<ecs::Registry>();
}

void SystemOrchestrator::update(FrameContext& frame)
{
    auto& scheduler = get_dependency<SchedulerModule>().get_scheduler();

    PORTAL_ASSERT(frame.active_registry != nullptr, "Invalid registry, cannot run systems");
    player_input_system->_execute(frame, *frame.ecs_registry, scheduler, nullptr);
    camera_system->_execute(frame, *frame.ecs_registry, scheduler, nullptr);
    transform_system->_execute(frame, *frame.ecs_registry, scheduler, nullptr);
    scene_rendering_system->_execute(frame, *frame.ecs_registry, scheduler, nullptr);
}
} // portal
