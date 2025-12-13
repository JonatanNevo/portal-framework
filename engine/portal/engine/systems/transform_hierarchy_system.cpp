//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "transform_hierarchy_system.h"

#include "portal/engine/scene/components/base.h"
#include "portal/engine/scene/components/relationship.h"

namespace portal
{
TransformHierarchySystem::TransformHierarchySystem(entt::registry& registry, jobs::Scheduler& scheduler)
    : System (registry, scheduler)
{
}

void TransformHierarchySystem::execute() const
{
    const auto transforms_to_update = group();
    transforms_to_update.sort(
        [this](const entt::entity lhs, const entt::entity rhs)
        {
            const auto& left_rel = registry.get<RelationshipComponent>(lhs);
            const auto& right_rel = registry.get<RelationshipComponent>(rhs);

            return right_rel.parent == lhs
                || left_rel.next == rhs
                || (!(left_rel.parent == rhs || right_rel.next == lhs)
                    && (left_rel.parent < right_rel.parent || (left_rel.parent == right_rel.parent && &left_rel < &right_rel)));
        }
    );

    for (auto&& [entity, transform] : transforms_to_update.each())
    {
        const auto& relationship = registry.get<RelationshipComponent>(entity);

        auto parent_matrix = glm::mat4(1.0f);
        if (relationship.parent != entt::null)
        {
            auto& parent_transform = registry.get<TransformComponent>(relationship.parent);
            parent_matrix = parent_transform.get_world_matrix();
        }
        transform.calculate_world_matrix(parent_matrix);
    }

    registry.clear<Dirty>();
}

void TransformHierarchySystem::on_component_added(const entt::entity entity, TransformComponent&) const
{
    registry.emplace_or_replace<Dirty>(entity);
}

void TransformHierarchySystem::on_component_removed(const entt::entity entity, TransformComponent&) const
{
    registry.emplace_or_replace<Dirty>(entity);
}

void TransformHierarchySystem::on_component_changed(const entt::entity entity, TransformComponent&) const
{
    registry.emplace_or_replace<Dirty>(entity);
}
} // portal
