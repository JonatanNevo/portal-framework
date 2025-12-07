//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "entity.h"

#include "components/base.h"
#include "components/relationship.h"
#include "entity_iterators.h"

namespace portal
{
Entity::Entity(const entt::entity entity, entt::registry& reg) : handle(reg, entity) {}

Entity::Entity(const entt::handle handle) : handle(handle) {}

void Entity::set_parent(const Entity parent)
{
    auto current_parent = get_parent();
    if (current_parent == parent)
        return;

    if (current_parent)
        current_parent.remove_child(*this);

    auto& relationship = get_component<RelationshipComponent>();
    relationship.parent = parent.get_id();

    if (parent)
    {
        auto& parent_rel = handle.registry()->get<RelationshipComponent>(parent.get_id());

        if (parent_rel.children == 0)
        {
            parent_rel.first = get_id();
        }
        else
        {
            auto last_child = parent_rel.first;
            for (size_t i = 1; i < parent_rel.children; ++i)
            {
                last_child = handle.registry()->get<RelationshipComponent>(last_child).next;
            }
            handle.registry()->get<RelationshipComponent>(last_child).next = get_id();
            relationship.prev = last_child;
        }
        parent_rel.children += 1;
        relationship.next = entt::null;
    }
    else
    {
        relationship.prev = entt::null;
        relationship.next = entt::null;
    }
}

bool Entity::remove_child(const Entity child)
{
    const auto child_id = child.get_id();
    auto& child_rel = handle.registry()->get<RelationshipComponent>(child_id);
    auto& parent_rel = get_component<RelationshipComponent>();

    if (child_rel.parent != get_id())
        return false;

    if (child_rel.prev != entt::null)
        handle.registry()->get<RelationshipComponent>(child_rel.prev).next = child_rel.next;
    else
        parent_rel.first = child_rel.next;

    if (child_rel.next != entt::null)
        handle.registry()->get<RelationshipComponent>(child_rel.next).prev = child_rel.prev;

    child_rel.prev = entt::null;
    child_rel.next = entt::null;
    child_rel.parent = entt::null;

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
    return has_component<TagComponent>() ? get_component<TagComponent>().tag : no_name;
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
