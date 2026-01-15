//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "selection_manager.h"

#include "portal/engine/components/selection.h"

namespace portal
{
void SelectionSystem::select(Entity entity, Entity scope)
{
    if (scope.has_component<SelectionComponent>())
        scope.patch_component<SelectionComponent>([entity](auto& comp) { comp.selected_entity = entity; });
    else
        scope.add_component<SelectionComponent>(entity);
}

bool SelectionSystem::is_selected(const Entity entity)
{
    auto& registry = entity.get_registry();
    for (auto [entity_id, selected] : registry.view<SelectionComponent>().each())
    {
        if (entity_id == entity.get_id())
            return true;
    }
    return false;
}

bool SelectionSystem::is_selected(const Entity entity, Entity scope)
{
    if (scope.has_component<SelectionComponent>())
        return scope.get_component<SelectionComponent>().selected_entity == entity;
    return false;
}

bool SelectionSystem::has_selection(Entity scope)
{
    return scope.has_component<SelectionComponent>();
}

Entity SelectionSystem::get_selected_entity(Entity scope)
{
    if (has_selection(scope))
        return scope.get_component<SelectionComponent>().selected_entity;
    return null_entity;
}

void SelectionSystem::deselect(const Entity entity)
{
    auto& registry = entity.get_registry();
    std::unordered_set<entt::entity> entities_to_remove;
    for (auto [entity_id, selected] : registry.view<SelectionComponent>().each())
    {
        if (selected.selected_entity == entity)
            entities_to_remove.insert(entity_id);
    }

    for (auto& entity_id : entities_to_remove)
        registry.remove<SelectionComponent>(entity_id);
}

void SelectionSystem::deselect(const Entity entity, Entity scope)
{
    if (scope.has_component<SelectionComponent>())
    {
        if (scope.get_component<SelectionComponent>().selected_entity == entity)
            scope.remove_component<SelectionComponent>();
    }
}
} // portal
