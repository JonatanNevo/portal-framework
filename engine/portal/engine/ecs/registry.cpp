//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "registry.h"

#include "portal/core/debug/profile.h"
#include "portal/engine/components/base.h"
#include "portal/engine/components/relationship.h"
#include "portal/engine/components/transform.h"

namespace portal::ecs
{
static auto logger = Log::get_logger("ECS Registry");

Registry::Registry()
{
    // Entity that holds global values
    env_entity = registry.create();
    registry.emplace<NameComponent>(env_entity, STRING_ID("env"));
    registry.emplace<RelationshipComponent>(env_entity);

    // All entities should have a relationship component, except the `scene` which is used as a global state
    add_default_component<RelationshipComponent>();
    add_default_component<TransformComponent>();
}

Registry::~Registry()
{
    clear();
}

Entity Registry::entity_from_id(const entt::entity id)
{
    if (id == entt::null)
        return Entity{entt::handle{}};
    return Entity{id, registry};
}

Entity Registry::find_or_create(const StringId& name)
{
    for (auto&& [entity, tag] : view_raw<NameComponent>().each())
    {
        if (tag.name == name)
            return entity_from_id(entity);
    }

    return create_entity(name);
}

Entity Registry::create_entity(const StringId& name)
{
    return create_child_entity({entt::handle{}}, name);
}

Entity Registry::find_or_create_child(const Entity parent, const StringId& name)
{
    for (auto&& [entity, tag] : view_raw<NameComponent>().each())
    {
        if (tag.name == name)
            return entity_from_id(entity);
    }

    return create_child_entity(parent, name);
}

Entity Registry::create_child_entity(const Entity parent, const StringId& name)
{
    PORTAL_PROF_ZONE();

    auto entity = Entity{registry.create(), registry};

    if (name != INVALID_STRING_ID)
        entity.add_component<NameComponent>(name);

    if (parent)
        entity.set_parent(parent);

    return entity;
}

Entity Registry::get_env_entity() const
{
    return Entity{env_entity, const_cast<entt::registry&>(registry)};
}

void Registry::destroy_entity(const Entity entity, const bool exclude_children)
{
    PORTAL_PROF_ZONE();

    if (!entity)
        return;

    // Destroy child entities (if relevant) before we start destroying this entity
    if (!exclude_children)
    {
        for (const auto child : entity.children())
        {
            destroy_entity(child, exclude_children);
        }
    }

    if (auto parent = entity.get_parent(); parent)
    {
        parent.remove_child(entity);
    }

    // TODO: destroy components with custom deleter callback to ensure order
    registry.destroy(entity);
}

void Registry::clear()
{
    for (const auto entity : view<entt::entity>())
    {
        if (registry.valid(entity))
            destroy_entity(entity, true);
    }

    registry.clear();
}
} // portal
