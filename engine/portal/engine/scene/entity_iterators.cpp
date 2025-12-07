//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "entity_iterators.h"
#include "entity.h"
#include "components/relationship.h"

namespace portal
{
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
        current = rel.next;
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
            llvm::SmallVector<entt::entity, 8> temp;
            auto child = rel.first;
            while (child != entt::null)
            {
                temp.push_back(child);
                const auto& child_rel = registry->get<RelationshipComponent>(child);
                child = child_rel.next;
            }

            for (auto it = temp.rbegin(); it != temp.rend(); ++it)
            {
                stack.push_back(*it);
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
        llvm::SmallVector<entt::entity, 8> temp;
        auto child = rel.first;
        while (child != entt::null)
        {
            temp.push_back(child);
            const auto& child_rel = registry->get<RelationshipComponent>(child);
            child = child_rel.next;
        }

        for (auto it = temp.rbegin(); it != temp.rend(); ++it)
        {
            stack.push_back(*it);
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
    return ChildIterator(rel.first, &entity.get_registry());
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
} // namespace portal
