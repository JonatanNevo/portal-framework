//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "entity.h"

#include "portal/engine/components/base.h"
#include "portal/engine/components/relationship.h"

namespace portal
{
Entity::Entity(const entt::entity entity, entt::registry& reg) : handle(reg, entity) {}

Entity::Entity(const entt::handle handle) : handle(handle) {}

void Entity::set_parent(const Entity parent) const
{
    auto current_parent = get_parent();
    if (current_parent == parent)
        return;

    if (current_parent)
        current_parent.remove_child(*this);

    auto& relationship = get_or_add_component<RelationshipComponent>();
    relationship.parent = parent;

    if (parent)
    {
        auto& parent_rel = parent.get_or_add_component<RelationshipComponent>();

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
    if (!has_component<RelationshipComponent>())
    {
        PORTAL_ASSERT(false, "Entity does not have a RelationshipComponent");
        return false;
    }

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
    auto& comp = get_or_add_component<RelationshipComponent>();
    return {comp.parent, *handle.registry()};
}

entt::entity Entity::get_parent_id() const
{
    return get_component<RelationshipComponent>().parent;
}

ChildRange Entity::children() const
{
    PORTAL_ASSERT(has_component<RelationshipComponent>(), "Entity does not have a RelationshipComponent");
    return ChildRange(*this);
}

bool Entity::has_children() const
{
    if (!has_component<RelationshipComponent>())
        return false;

    return get_component<RelationshipComponent>().children > 0;
}

RecursiveChildRange Entity::descendants() const
{
    PORTAL_ASSERT(has_component<RelationshipComponent>(), "Entity does not have a RelationshipComponent");
    return RecursiveChildRange(*this);
}

size_t Entity::descendants_count() const
{
    // TODO: this is super not efficient
    PORTAL_ASSERT(has_component<RelationshipComponent>(), "Entity does not have a RelationshipComponent");
    size_t count = 0;
    for ([[maybe_unused]] auto _ : descendants())
        ++count;
    return count;
}

bool Entity::is_ancestor_of(const Entity other) const
{
    if (!has_component<RelationshipComponent>())
    {
        PORTAL_ASSERT(false, "Entity does not have a RelationshipComponent");
        return false;
    }

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

ChildIterator::ChildIterator(const entt::entity current, entt::registry* registry)
    : current(current), registry(registry)
{}

Entity ChildIterator::operator*() const
{
    return Entity(current, *registry);
}

Entity ChildIterator::operator->() const
{
    return Entity(current, *registry);
}

ChildIterator& ChildIterator::operator++()
{
    if (current != entt::null)
    {
        const auto& rel = registry->get<RelationshipComponent>(current);
        current = rel.next.get_id();
    }
    return *this;
}

ChildIterator ChildIterator::operator++(int)
{
    const ChildIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool ChildIterator::operator==(const ChildIterator& other) const
{
    return current == other.current && registry == other.registry;
}

bool ChildIterator::operator!=(const ChildIterator& other) const
{
    return !(*this == other);
}

RecursiveChildIterator::RecursiveChildIterator(const entt::entity start, entt::registry* registry, const bool is_end)
    : current(is_end ? entt::null : start), registry(registry)
{
    if (!is_end && start != entt::null)
    {
        const auto& rel = registry->get<RelationshipComponent>(start);
        if (rel.children > 0)
        {
            llvm::SmallVector<Entity, 8> temp;
            auto child = rel.first;
            while (child.get_id() != entt::null)
            {
                temp.push_back(child);
                const auto& child_rel = child.get_component<RelationshipComponent>();
                child = child_rel.next;
            }

            for (auto it = temp.rbegin(); it != temp.rend(); ++it)
            {
                stack.push_back(it->get_id());
            }
        }

        advance_to_next();
    }
}

void RecursiveChildIterator::advance_to_next()
{
    if (stack.empty())
    {
        current = entt::null;
        return;
    }

    current = stack.back();
    stack.pop_back();

    const auto& rel = registry->get<RelationshipComponent>(current);
    if (rel.children > 0)
    {
        llvm::SmallVector<Entity, 8> temp;
        auto child = rel.first;
        while (child.get_id() != entt::null)
        {
            temp.push_back(child);
            const auto& child_rel = child.get_component<RelationshipComponent>();
            child = child_rel.next;
        }

        for (auto it = temp.rbegin(); it != temp.rend(); ++it)
        {
            stack.push_back(it->get_id());
        }
    }
}

Entity RecursiveChildIterator::operator*() const
{
    return Entity(current, *registry);
}

Entity RecursiveChildIterator::operator->() const
{
    return Entity(current, *registry);
}

RecursiveChildIterator& RecursiveChildIterator::operator++()
{
    advance_to_next();
    return *this;
}

RecursiveChildIterator RecursiveChildIterator::operator++(int)
{
    RecursiveChildIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool RecursiveChildIterator::operator==(const RecursiveChildIterator& other) const
{
    return current == other.current && registry == other.registry;
}

bool RecursiveChildIterator::operator!=(const RecursiveChildIterator& other) const
{
    return !(*this == other);
}


ChildRange::ChildRange(const Entity& entity) : entity(entity)
{
}

ChildIterator ChildRange::begin() const
{
    const auto& rel = entity.get_component<RelationshipComponent>();
    return ChildIterator(rel.first.get_id(), &entity.get_registry());
}

ChildIterator ChildRange::end() const
{
    return ChildIterator(entt::null, &entity.get_registry());
}

RecursiveChildRange::RecursiveChildRange(const Entity& entity) : entity(entity)
{
}

RecursiveChildIterator RecursiveChildRange::begin() const
{
    return RecursiveChildIterator(
        entity.get_id(),
        &entity.get_registry(),
        false
    );
}

RecursiveChildIterator RecursiveChildRange::end() const
{
    return RecursiveChildIterator(
        entt::null,
        &entity.get_registry(),
        true
    );
}
} // portal
