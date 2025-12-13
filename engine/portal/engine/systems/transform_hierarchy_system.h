//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "system.h"

#include "portal/engine/scene/components/base.h"
#include "portal/engine/scene/components/transform.h"

namespace portal
{
class TransformHierarchySystem : public System<TransformHierarchySystem, Dirty, TransformComponent>
{
public:
    TransformHierarchySystem(entt::registry& registry, jobs::Scheduler& scheduler);

    void execute() const;

    void on_component_added(entt::entity entity, TransformComponent& transform) const;
    void on_component_removed(entt::entity entity, TransformComponent& transform) const;
    void on_component_changed(entt::entity entity, TransformComponent& transform) const;

    [[nodiscard]] static StringId get_name() { return STRING_ID("Transform Hierarchy"); };
};
} // portal
