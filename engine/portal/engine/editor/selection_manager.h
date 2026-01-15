//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/ecs/entity.h"

namespace portal
{

/**
 * @brief Static utility class for managing entity selection state in the editor.
 *
 * SelectionSystem provides scoped entity selection, where selections are tied to a
 * scope entity (typically a scene). This allows different scenes or contexts to
 * maintain independent selection states.
 */
class SelectionSystem
{
public:
    /**
     * @brief Selects an entity within a given scope.
     * @param entity The entity to select.
     * @param scope The scope entity (e.g., scene) the selection belongs to.
     */
    static void select(Entity entity, Entity scope);

    /**
     * @brief Checks if an entity is selected in any scope.
     * @param entity The entity to check.
     * @return True if the entity is selected.
     */
    static bool is_selected(Entity entity);

    /**
     * @brief Checks if an entity is selected within a specific scope.
     * @param entity The entity to check.
     * @param scope The scope to check within.
     * @return True if the entity is selected in the given scope.
     */
    static bool is_selected(Entity entity, Entity scope);

    /**
     * @brief Checks if there is any selection within a scope.
     * @param scope The scope to check.
     * @return True if any entity is selected in the scope.
     */
    static bool has_selection(Entity scope);

    /**
     * @brief Gets the currently selected entity in a scope.
     * @param scope The scope to query.
     * @return The selected entity, or an invalid entity if none selected.
     */
    static Entity get_selected_entity(Entity scope);

    /**
     * @brief Deselects an entity from all scopes.
     * @param entity The entity to deselect.
     */
    static void deselect(Entity entity);

    /**
     * @brief Deselects an entity from a specific scope.
     * @param entity The entity to deselect.
     * @param scope The scope to deselect from.
     */
    static void deselect(Entity entity, Entity scope);
};

} // portal