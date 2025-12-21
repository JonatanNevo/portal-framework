//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/ecs/system.h"

#include "portal/engine/components/transform.h"

namespace portal
{
class TransformHierarchySystem final : public ecs::System<TransformHierarchySystem, ecs::Owns<TransformDirtyTag>, ecs::Owns<TransformComponent>>
{
public:
    static void execute(ecs::Registry& registry);

    static void on_component_added(Entity entity, TransformComponent& transform);
    static void on_component_changed(Entity entity, TransformComponent& transform);

    [[nodiscard]] static StringId get_name() { return STRING_ID("Transform Hierarchy"); };
};
} // portal
