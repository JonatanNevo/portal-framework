//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/ecs/system.h"
#include "portal/engine/components/mesh.h"
#include "portal/engine/components/transform.h"

namespace portal
{
class SceneRenderingSystem : public ecs::System<SceneRenderingSystem, ecs::Owns<StaticMeshComponent>, ecs::Views<TransformComponent>>
{
public:
    static void execute(FrameContext& frame, ecs::Registry& registry);

    static void update_global_descriptors(FrameContext& frame, ecs::Registry& registry);
    static void add_static_mesh_to_context(FrameContext& frame, ecs::Registry& registry);

    [[nodiscard]] static StringId get_name() { return STRING_ID("Scene Rendering"); };
};
} // portal
