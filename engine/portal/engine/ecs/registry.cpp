//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "registry.h"

#include "portal/application/modules/module_stack.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/components/base.h"
#include "portal/engine/components/relationship.h"
#include "portal/engine/components/transform.h"

namespace portal::ecs
{
static auto logger = Log::get_logger("ECS Registry");

Registry::Registry(ModuleStack& stack) : TaggedModule(stack, STRING_ID("ECS Registry")), registry(), env_entity(registry.create())
{
    // Entity that holds global values
    registry.emplace<NameComponent>(env_entity, STRING_ID(ENV_ENTITY_ID));
    registry.emplace<RelationshipComponent>(env_entity);
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

Entity Registry::find_or_create(const StringId& entity_name)
{
    for (auto&& [entity, tag] : view_raw<NameComponent>().each())
    {
        if (tag.name == entity_name)
            return entity_from_id(entity);
    }

    return create_entity(entity_name);
}

Entity Registry::get_env_entity() const
{
    return Entity{env_entity, const_cast<entt::registry&>(registry)};
}

Entity Registry::find_or_create_child(Entity parent, const StringId& entity_name)
{
    PORTAL_PROF_ZONE();

    for (auto&& [entity, tag] : view_raw<NameComponent>().each())
    {
        if (tag.name == entity_name)
            return entity_from_id(entity);
    }

    return create_child_entity(parent, entity_name);
}

Entity Registry::create_child_entity(Entity parent, const StringId& entity_name)
{
    PORTAL_PROF_ZONE();

    Entity child;
    if (entity_name != INVALID_STRING_ID)
        child = make_entity<NameComponent, RelationshipComponent>(
            NameComponent{entity_name},
            RelationshipComponent{}
        );
    else
        child = make_entity<RelationshipComponent>(RelationshipComponent{});

    if (parent)
        child.set_parent(parent);

    return child;
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
