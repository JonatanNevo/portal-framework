//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "system_orchestrator.h"

namespace portal
{
SystemOrchestrator::SystemOrchestrator(ModuleStack& stack, ecs::Registry& registry)
    : TaggedModule(stack, STRING_ID("System Orchestrator")), registry(registry)
{
    player_input_system.register_to(registry);
    camera_system.register_to(registry);
    transform_system.register_to(registry);
    scene_rendering_system.register_to(registry);
}

void SystemOrchestrator::update(FrameContext& frame)
{
    auto& scheduler = get_dependency<SchedulerModule>().get_scheduler();

    player_input_system._execute(frame, registry, scheduler, nullptr);
    camera_system._execute(frame, registry, scheduler, nullptr);
    transform_system._execute(frame, registry, scheduler, nullptr);
    scene_rendering_system._execute(frame, registry, scheduler, nullptr);
}
} // portal
