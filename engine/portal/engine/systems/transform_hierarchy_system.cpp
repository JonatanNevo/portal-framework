//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "transform_hierarchy_system.h"

#include "portal/engine/components/base.h"
#include "portal/engine/components/relationship.h"

namespace portal
{
void TransformHierarchySystem::execute(ecs::Registry& registry)
{
    const auto transforms_to_update = group(registry);
    transforms_to_update.sort(
        [&registry](const entt::entity lhs_raw, const entt::entity rhs_raw)
        {
            auto lhs = registry.entity_from_id(lhs_raw);
            auto rhs = registry.entity_from_id(rhs_raw);

            const auto& left_rel = lhs.get_component<RelationshipComponent>();
            const auto& right_rel = rhs.get_component<RelationshipComponent>();

            return right_rel.parent == lhs
                || left_rel.next == rhs
                || (!(left_rel.parent == rhs || right_rel.next == lhs)
                    && (left_rel.parent.get_id() < right_rel.parent.get_id() || (left_rel.parent == right_rel.parent && &left_rel < &right_rel)));
        }
    );

    for (auto&& [entity_raw, transform] : transforms_to_update.each())
    {
        auto entity = registry.entity_from_id(entity_raw);
        const auto& relationship = entity.get_component<RelationshipComponent>();

        auto parent_matrix = glm::mat4(1.0f);
        if (relationship.parent != null_entity)
        {
            auto& parent_transform = relationship.parent.get_component<TransformComponent>();
            parent_matrix = parent_transform.get_world_matrix();
        }
        transform.calculate_world_matrix(parent_matrix);
    }

    registry.clear<TransformDirtyTag>();
}

void TransformHierarchySystem::on_component_added(ecs::Registry& registry, const Entity entity, TransformComponent&)
{
    registry.get_raw_registry().emplace_or_replace<TransformDirtyTag>(entity);
}

void TransformHierarchySystem::on_component_removed(ecs::Registry& registry, const Entity entity, TransformComponent&)
{
    registry.get_raw_registry().emplace_or_replace<TransformDirtyTag>(entity);
}

void TransformHierarchySystem::on_component_changed(ecs::Registry& registry, const Entity entity, TransformComponent&)
{
    registry.get_raw_registry().emplace_or_replace<TransformDirtyTag>(entity);
}
} // portal
