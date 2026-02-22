//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <span>
#include <vector>

#include "portal/core/strings/string_id.h"
#include "portal/engine/ecs/entity.h"
#include "portal/engine/ecs/registry.h"

namespace portal
{

/**
 * @brief Static utility class for managing selection state in the editor.
 *
 * SelectionSystem provides scoped selection using StringId identifiers, where selections
 * are tied to a scope entity (typically a scene). This allows different scenes or contexts
 * to maintain independent selection states. StringId-based selection generalizes beyond
 * entities to support selecting resources, paths, or any named item.
 *
 * Selections are additive by default. To replace the current selection, call
 * deselect_all() before select().
 */
class SelectionSystem
{
public:
    /**
     * @brief Adds an item to the selection within a given scope.
     * @param id The StringId of the item to select.
     * @param scope The scope entity (e.g., scene) the selection belongs to.
     */
    static void select(StringId id, Entity scope);

    /**
     * @brief Adds an entity to the selection within a given scope (by name).
     * @param entity The entity to select.
     * @param scope The scope entity (e.g., scene) the selection belongs to.
     */
    static void select(Entity entity, Entity scope);

    /**
     * @brief Selects multiple items within a given scope, skipping duplicates.
     * @param ids The StringIds to select.
     * @param scope The scope entity the selection belongs to.
     */
    static void select_all(std::span<const StringId> ids, Entity scope);

    /**
     * @brief Checks if an item is selected within a specific scope.
     * @param id The StringId to check.
     * @param scope The scope to check within.
     * @return True if the item is selected in the given scope.
     */
    static bool is_selected(const StringId& id, Entity scope);

    /**
     * @brief Checks if an entity is selected within a specific scope (by name).
     * @param entity The entity to check.
     * @param scope The scope to check within.
     * @return True if the entity is selected in the given scope.
     */
    static bool is_selected(Entity entity, Entity scope);

    /**
     * @brief Checks if there is any selection within a scope.
     * @param scope The scope to check.
     * @return True if any item is selected in the scope.
     */
    static bool has_selection(Entity scope);

    /**
     * @brief Gets the first selection in a scope.
     * @param scope The scope to query.
     * @return The first selected StringId, or an empty StringId if none selected.
     */
    static StringId get_selected(Entity scope);

    static StringId get_selection_by_index(Entity scope, size_t index);

    /**
     * @brief Gets all selections in a scope.
     * @param scope The scope to query.
     * @return A const reference to the vector of selected StringIds.
     */
    static const std::vector<StringId>& get_selections(Entity scope);

    /**
     * @brief Returns the number of selections in a scope.
     * @param scope The scope to query.
     * @return The number of selections.
     */
    static size_t selection_count(Entity scope);

    /**
     * @brief Deselects an item from all scopes in the given registry.
     * @param id The StringId to deselect.
     * @param registry The registry to search for selection scopes.
     */
    static void deselect(const StringId& id, ecs::Registry& registry);

    /**
     * @brief Deselects an item from a specific scope.
     * @param id The StringId to deselect.
     * @param scope The scope to deselect from.
     */
    static void deselect(StringId id, Entity scope);

    /**
     * @brief Clears all selections in a scope.
     * @param scope The scope to clear.
     */
    static void deselect_all(Entity scope);

    /**
     * @brief Resolves a StringId selection to an Entity via the scope's registry.
     * @param id The StringId to resolve.
     * @param scope The scope entity whose registry is used for lookup.
     * @return The resolved Entity.
     */
    static Entity selection_to_entity(const StringId& id, Entity scope);

    /**
     * @brief Convenience: gets the first selection as an Entity using the scope's registry.
     * @param scope The scope to query.
     * @return The first selected entity, or null_entity if none selected.
     */
    static Entity get_selected_entity(Entity scope);
};

} // portal