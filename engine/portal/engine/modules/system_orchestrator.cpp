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
    : TaggedModule(stack, STRING_ID("System Orchestrator"))
{
}

void SystemOrchestrator::clean()
{
    active_scene = {};
}

void SystemOrchestrator::connect(entt::dispatcher& dispatcher)
{
    auto& registry = get_dependency<ecs::Registry>();

    for (const auto& system: input_systems)
        system->connect(registry, dispatcher);
    for (const auto& system: update_systems)
        system->connect(registry, dispatcher);
    for (const auto& system: fixed_update_systems)
        system->connect(registry, dispatcher);
    for (const auto& system: pre_render_systems)
        system->connect(registry, dispatcher);
}

void SystemOrchestrator::disconnect(entt::dispatcher& dispatcher)
{
    auto& registry = get_dependency<ecs::Registry>();

    for (const auto& system: input_systems)
        system->disconnect(registry, dispatcher);
    for (const auto& system: update_systems)
        system->disconnect(registry, dispatcher);
    for (const auto& system: fixed_update_systems)
        system->disconnect(registry, dispatcher);
    for (const auto& system: pre_render_systems)
        system->disconnect(registry, dispatcher);
}

void SystemOrchestrator::set_active_scene(const ResourceReference<Scene>& scene)
{
    active_scene = scene;
}

void SystemOrchestrator::begin_frame(FrameContext& frame)
{
    PORTAL_ASSERT(active_scene.get_state() == ResourceState::Loaded, "Invalid scene, cannot run systems");
    frame.ecs_registry = &get_dependency<ecs::Registry>();
    PORTAL_ASSERT(frame.ecs_registry != nullptr, "Invalid registry, cannot run systems");


    auto& scheduler = get_dependency<SchedulerModule>().get_scheduler();
    for (const auto& system: input_systems)
    {
        system->execute_erased(frame, *frame.ecs_registry, scheduler, nullptr);
    }
}

void SystemOrchestrator::update(FrameContext& frame)
{
    auto& scheduler = get_dependency<SchedulerModule>().get_scheduler();

    PORTAL_ASSERT(frame.ecs_registry != nullptr, "Invalid registry, cannot run systems");
    for (const auto& system: update_systems)
    {
        system->execute_erased(frame, *frame.ecs_registry, scheduler, nullptr);
    }
}

void SystemOrchestrator::fixed_update(FrameContext& frame)
{
    auto& scheduler = get_dependency<SchedulerModule>().get_scheduler();

    PORTAL_ASSERT(frame.ecs_registry != nullptr, "Invalid registry, cannot run systems");
    for (const auto& system: fixed_update_systems)
    {
        system->execute_erased(frame, *frame.ecs_registry, scheduler, nullptr);
    }
}

void SystemOrchestrator::post_update(FrameContext& frame)
{
    auto& scheduler = get_dependency<SchedulerModule>().get_scheduler();

    PORTAL_ASSERT(frame.ecs_registry != nullptr, "Invalid registry, cannot run systems");
    for (const auto& system: pre_render_systems)
    {
        system->execute_erased(frame, *frame.ecs_registry, scheduler, nullptr);
    }
}

} // portal
