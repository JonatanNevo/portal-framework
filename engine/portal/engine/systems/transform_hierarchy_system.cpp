//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "transform_hierarchy_system.h"

#include "portal/engine/components/base.h"
#include "portal/engine/components/relationship.h"
#include "portal/core/log.h"

namespace portal
{
void TransformHierarchySystem::execute(ecs::Registry& registry)
{
    const auto transforms_to_update = group(registry);

    auto get_depth = [&registry](entt::entity e) -> int
    {
        int depth = 0;
        auto entity = registry.entity_from_id(e);
        while (entity.get_component<RelationshipComponent>().parent != null_entity)
        {
            entity = entity.get_component<RelationshipComponent>().parent;
            ++depth;
        }
        return depth;
    };

    transforms_to_update.sort(
        [&get_depth](const entt::entity lhs_raw, const entt::entity rhs_raw)
        {
            const int left_depth = get_depth(lhs_raw);
            const int right_depth = get_depth(rhs_raw);

            if (left_depth != right_depth)
                return left_depth < right_depth;

            return lhs_raw < rhs_raw;
        }
    );

    for (auto&& [entity_raw, transform, relationship] : transforms_to_update.each())
    {
        auto parent_matrix = glm::mat4(1.0f);
        if (relationship.parent != null_entity)
        {
            if (relationship.parent.has_component<TransformComponent>())
            {
                auto& parent_transform = relationship.parent.get_component<TransformComponent>();
                parent_matrix = parent_transform.get_world_matrix();
            }
            else
            {
                parent_matrix = glm::mat4(1.0f);
            }

        }
        transform.calculate_world_matrix(parent_matrix);
    }

    registry.clear<TransformDirtyTag>();
}

void TransformHierarchySystem::on_component_added(const Entity entity, TransformComponent&)
{
    entity.get_registry().emplace_or_replace<TransformDirtyTag>(entity);
}

void TransformHierarchySystem::on_component_changed(const Entity entity, TransformComponent&)
{
    entity.get_registry().emplace_or_replace<TransformDirtyTag>(entity);
}
} // portal
