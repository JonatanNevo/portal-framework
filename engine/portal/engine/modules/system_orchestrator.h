//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/systems/base_camera_system.h"
#include "portal/engine/systems/base_player_input_system.h"
#include "portal/engine/systems/scene_rendering_system.h"
#include "portal/engine/systems/transform_hierarchy_system.h"

namespace portal
{


// TODO: support dependencies between systems and parallel system execution
// TODO: support dynamic system declaration
// TODO: support creating system stack from file
// TODO: have the registry as a module as well?
class SystemOrchestrator final: public TaggedModule<Tag<ModuleTags::Update>, SchedulerModule, Renderer, ResourceRegistry>
{
public:
    explicit SystemOrchestrator(ModuleStack& stack, ecs::Registry& registry);

    void update(FrameContext& frame) override;

private:
    ecs::Registry& registry;

    BasePlayerInputSystem player_input_system;
    BaseCameraSystem camera_system;
    TransformHierarchySystem transform_system;
    SceneRenderingSystem scene_rendering_system;
};

} // portal