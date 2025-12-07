//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <entt/entt.hpp>

#include "llvm/ADT/SmallVector.h"
#include "portal/core/uuid.h"
#include "portal/core/debug/assert.h"
#include "portal/core/strings/string_id.h"

#include "entity_iterators.h"

namespace portal
{

namespace ng
{
    class Scene;
}

/**
 * A wrapper around entt::handle with relationship management
 */
class Entity
{
public:
    Entity(entt::entity entity, entt::registry& reg);
    Entity(entt::handle handle);

    template <typename T, typename... Args>
    T& add_component(Args&&... args)
    {
        PORTAL_ASSERT(!has_component<T>(), "Entity already has component of type T");
        return handle.emplace<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    void remove_component() const
    {
        PORTAL_ASSERT(has_component<T>(), "Entity does not have component of type T");
        auto deleted = handle.remove<T>();
        PORTAL_ASSERT(deleted == 1, "Failed to remove component of type T");
    }

    void set_parent(Entity parent);
    bool remove_child(Entity child);

    template <typename T>
    [[nodiscard]] T& get_component()
    {
        PORTAL_ASSERT(has_component<T>(), "Entity does not have component of type T");
        return handle.get<T>();
    }

    template <typename T>
    [[nodiscard]] const T& get_component() const
    {
        PORTAL_ASSERT(has_component<T>(), "Entity does not have component of type T");
        return handle.get<T>();
    }

    template <typename T>
    [[nodiscard]] T* try_get_component()
    {
        PORTAL_ASSERT(is_valid(), "Entity is invalid");
        return handle.try_get<T>();
    }

    template <typename T>
    [[nodiscard]] const T* try_get_component() const
    {
        PORTAL_ASSERT(is_valid(), "Entity is invalid");
        return handle.try_get<T>();
    }

    template <typename... T>
    [[nodiscard]] bool has_component() const
    {
        PORTAL_ASSERT(is_valid(), "Entity is invalid");
        return handle.all_of<T...>();
    }

    template <typename... T>
    [[nodiscard]] bool has_any() const
    {
        PORTAL_ASSERT(is_valid(), "Entity is invalid");

        return handle.any_of<T...>();
    }

    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] entt::entity get_id() const;
    [[nodiscard]] StringId get_name() const;

    [[nodiscard]] operator uint32_t() const;
    [[nodiscard]] operator entt::entity() const;
    [[nodiscard]] operator bool() const;
    [[nodiscard]] bool operator==(const Entity& other) const;

    [[nodiscard]] Entity get_parent() const;
    [[nodiscard]] entt::entity get_parent_id() const;

    [[nodiscard]] ChildRange children() const;
    [[nodiscard]] RecursiveChildRange descendants() const;

    [[nodiscard]] bool is_ancestor_of(Entity other) const;
    [[nodiscard]] bool is_descendant_of(Entity other) const;

    [[nodiscard]] entt::registry& get_registry() const;

private:
    entt::handle handle;

    inline const static auto no_name = STRING_ID("Unnamed");
};
} // portal
