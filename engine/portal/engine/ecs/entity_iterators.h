//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <entt/entt.hpp>
#include "llvm/ADT/SmallVector.h"

namespace portal
{
class Entity;

/**
 * @brief Forward iterator for traversing direct children of an entity.
 *
 * ChildIterator provides STL-compliant forward iteration over an entity's immediate
 * children by following the sibling linked list in RelationshipComponent. This is a
 * non-recursive iteration - it only visits direct children, not descendants.
 *
 * The iterator wraps raw entt::entity IDs in Portal's Entity class for convenient access.
 *
 * @par Example:
 * @code
 * for (auto child : entity.children()) {
 *     // Only processes direct children
 *     child.get_component<TransformComponent>();
 * }
 * @endcode
 *
 * @see ChildRange for the range wrapper
 * @see RecursiveChildIterator for recursive traversal
 * @see RelationshipComponent for the underlying linked list structure
 */
class ChildIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Entity;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    /**
     * @brief Constructs an iterator at the specified child entity.
     *
     * @param current The current child entity ID
     * @param registry Pointer to the registry containing the entity
     */
    ChildIterator(entt::entity current, entt::registry* registry);

    /**
     * @brief Dereferences the iterator to access the current child entity.
     * @return The current child as an Entity wrapper
     */
    Entity operator*() const;

    /**
     * @brief Arrow operator for accessing the current child entity.
     * @return The current child as an Entity wrapper
     */
    Entity operator->() const;

    /**
     * @brief Advances to the next sibling.
     * @return Reference to this iterator
     */
    ChildIterator& operator++();

    /**
     * @brief Post-increment (advances to next sibling).
     * @return Copy of the iterator before advancement
     */
    ChildIterator operator++(int);

    /**
     * @brief Compares iterators for equality.
     * @param other The iterator to compare against
     * @return true if both iterators point to the same entity
     */
    bool operator==(const ChildIterator& other) const;

    /**
     * @brief Compares iterators for inequality.
     * @param other The iterator to compare against
     * @return true if iterators point to different entities
     */
    bool operator!=(const ChildIterator& other) const;

private:
    entt::entity current;
    entt::registry* registry;
};

/**
 * @brief Forward iterator for depth-first traversal of all descendants.
 *
 * RecursiveChildIterator provides STL-compliant forward iteration over an entity's
 * entire subtree using depth-first traversal. It maintains an internal stack to track
 * pending nodes, avoiding recursion overhead.
 *
 * The iterator uses llvm::SmallVector with an 8-entity inline capacity for the stack,
 * optimizing for typical hierarchy depths without heap allocations.
 *
 * @par Example:
 * @code
 * for (auto descendant : entity.descendants()) {
 *     // Processes entire subtree in depth-first order
 *     descendant.get_component<TransformComponent>();
 * }
 * @endcode
 *
 * @see RecursiveChildRange for the range wrapper
 * @see ChildIterator for non-recursive direct children iteration
 */
class RecursiveChildIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Entity;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    /**
     * @brief Constructs an iterator for recursive traversal.
     *
     * @param start The root entity to begin traversal from
     * @param registry Pointer to the registry containing entities
     * @param is_end If true, constructs an end sentinel iterator
     */
    RecursiveChildIterator(entt::entity start, entt::registry* registry, bool is_end = false);

    /**
     * @brief Dereferences the iterator to access the current descendant.
     * @return The current descendant as an Entity wrapper
     */
    Entity operator*() const;

    /**
     * @brief Arrow operator for accessing the current descendant.
     * @return The current descendant as an Entity wrapper
     */
    Entity operator->() const;

    /**
     * @brief Advances to the next descendant in depth-first order.
     * @return Reference to this iterator
     */
    RecursiveChildIterator& operator++();

    /**
     * @brief Post-increment (advances to next descendant).
     * @return Copy of the iterator before advancement
     */
    RecursiveChildIterator operator++(int);

    /**
     * @brief Compares iterators for equality.
     * @param other The iterator to compare against
     * @return true if both iterators point to the same entity
     */
    bool operator==(const RecursiveChildIterator& other) const;

    /**
     * @brief Compares iterators for inequality.
     * @param other The iterator to compare against
     * @return true if iterators point to different entities
     */
    bool operator!=(const RecursiveChildIterator& other) const;

private:
    void advance_to_next();

    entt::entity current;
    entt::registry* registry;
    llvm::SmallVector<entt::entity, 8> stack;
};

/**
 * @brief Range wrapper for iterating direct children.
 *
 * ChildRange provides a range-based for loop compatible wrapper around ChildIterator.
 * Returned by Entity::children().
 *
 * @par Example:
 * @code
 * for (auto child : entity.children()) {
 *     // Process direct children only
 * }
 * @endcode
 *
 * @see ChildIterator
 * @see Entity::children
 */
class ChildRange
{
public:
    /**
     * @brief Constructs a range for the entity's direct children.
     * @param entity The parent entity
     */
    ChildRange(const Entity& entity);

    /**
     * @brief Returns an iterator to the first child.
     * @return Iterator positioned at the first child, or end() if no children
     */
    ChildIterator begin() const;

    /**
     * @brief Returns a sentinel iterator marking the end of children.
     * @return End iterator
     */
    ChildIterator end() const;

private:
    const Entity& entity;
};

/**
 * @brief Range wrapper for iterating all descendants recursively.
 *
 * RecursiveChildRange provides a range-based for loop compatible wrapper around
 * RecursiveChildIterator. Returned by Entity::descendants().
 *
 * @par Example:
 * @code
 * for (auto descendant : entity.descendants()) {
 *     // Process entire subtree in depth-first order
 * }
 * @endcode
 *
 * @see RecursiveChildIterator
 * @see Entity::descendants
 */
class RecursiveChildRange
{
public:
    /**
     * @brief Constructs a range for the entity's entire subtree.
     * @param entity The root entity
     */
    RecursiveChildRange(const Entity& entity);

    /**
     * @brief Returns an iterator to the first descendant.
     * @return Iterator positioned at the first descendant (depth-first), or end() if no descendants
     */
    RecursiveChildIterator begin() const;

    /**
     * @brief Returns a sentinel iterator marking the end of descendants.
     * @return End iterator
     */
    RecursiveChildIterator end() const;

private:
    const Entity& entity;
};
} // namespace portal
