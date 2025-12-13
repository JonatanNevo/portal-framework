//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "entity.h"
#include "entity_iterators.h"

#include "portal/engine/components/base.h"
#include "portal/engine/components/relationship.h"

namespace portal
{
Entity::Entity(const entt::entity entity, entt::registry& reg) : handle(reg, entity) {}

Entity::Entity(const entt::handle handle) : handle(handle) {}

void Entity::set_parent(Entity parent)
{
    auto current_parent = get_parent();
    if (current_parent == parent)
        return;

    if (current_parent)
        current_parent.remove_child(*this);

    auto& relationship = get_component<RelationshipComponent>();
    relationship.parent = parent;

    if (parent)
    {
        auto& parent_rel = parent.get_component<RelationshipComponent>();

        if (parent_rel.children == 0)
        {
            parent_rel.first = *this;
        }
        else
        {
            auto last_child = parent_rel.first;
            for (size_t i = 1; i < parent_rel.children; ++i)
            {
                last_child = last_child.get_component<RelationshipComponent>().next;
            }
            last_child.get_component<RelationshipComponent>().next = *this;
            relationship.prev = last_child;
        }
        parent_rel.children += 1;
        relationship.next = null_entity;
    }
    else
    {
        relationship.prev = null_entity;
        relationship.next = null_entity;
    }
}

bool Entity::remove_child(Entity child)
{
    auto& child_rel = child.get_component<RelationshipComponent>();
    auto& parent_rel = get_component<RelationshipComponent>();

    if (child_rel.parent != *this)
        return false;

    if (child_rel.prev != null_entity)
        child_rel.prev.get_component<RelationshipComponent>().next = child_rel.next;
    else
        parent_rel.first = child_rel.next;

    if (child_rel.next != null_entity)
        child_rel.next.get_component<RelationshipComponent>().prev = child_rel.prev;

    child_rel.prev = null_entity;
    child_rel.next = null_entity;
    child_rel.parent = null_entity;

    parent_rel.children -= 1;
    return true;
}

bool Entity::is_valid() const
{
    return handle.valid();
}

entt::entity Entity::get_id() const
{
    return handle.entity();
}

StringId Entity::get_name() const
{
    return has_component<NameComponent>() ? get_component<NameComponent>().name : no_name;
}

Entity::operator uint32_t() const
{
    return static_cast<uint32_t>(handle.entity());
}

Entity::operator entt::entity() const
{
    return handle.entity();
}

Entity::operator bool() const
{
    return is_valid();
}

bool Entity::operator==(const Entity& other) const
{
    return handle == other.handle;
}

Entity Entity::get_parent() const
{
    return {get_component<RelationshipComponent>().parent, *handle.registry()};
}

entt::entity Entity::get_parent_id() const
{
    return get_component<RelationshipComponent>().parent;
}

ChildRange Entity::children() const
{
    return ChildRange(*this);
}

RecursiveChildRange Entity::descendants() const
{
    return RecursiveChildRange(*this);
}

bool Entity::is_ancestor_of(const Entity other) const
{
    auto& relationship = get_component<RelationshipComponent>();

    if (relationship.children == 0)
        return false;

    for (auto child : children())
    {
        if (child == other)
            return true;

        if (child.is_ancestor_of(other))
            return true;
    }

    return false;
}

bool Entity::is_descendant_of(const Entity other) const
{
    return other.is_ancestor_of(*this);
}

entt::registry& Entity::get_registry() const
{
    return *handle.registry();
}
} // portal
