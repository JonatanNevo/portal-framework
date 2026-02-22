//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "selection_system.h"

#include <algorithm>
#include <fmt/ranges.h>

#include "portal/engine/components/selection.h"

namespace portal
{
static auto logger = Log::get_logger("Selection System");

void SelectionSystem::select(StringId id, Entity scope)
{
    if (scope.has_component<SelectionComponent>())
    {
        scope.patch_component<SelectionComponent>(
            [id](auto& comp)
            {
                auto& vec = comp.selections;
                if (std::ranges::find(vec, id) == vec.end())
                    vec.push_back(id);
            }
        );
    }
    else
    {
        scope.add_component<SelectionComponent>(std::vector{id});
    }

    LOGGER_TRACE("Selected {} to {}", id, scope.get_name());
}

void SelectionSystem::select(const Entity entity, const Entity scope)
{
    select(entity.get_name(), scope);
}

void SelectionSystem::select_all(std::span<const StringId> ids, Entity scope)
{
    if (!scope.has_component<SelectionComponent>())
        scope.add_component<SelectionComponent>();

    scope.patch_component<SelectionComponent>(
        [&ids](auto& comp)
        {
            for (const auto& id : ids)
            {
                if (std::ranges::find(comp.selections, id) == comp.selections.end())
                    comp.selections.push_back(id);
            }
        }
    );

    LOGGER_TRACE("Selected {} to {}", ids, scope.get_name());
}

bool SelectionSystem::is_selected(const StringId& id, Entity scope)
{
    if (scope.has_component<SelectionComponent>())
    {
        const auto& vec = scope.get_component<SelectionComponent>().selections;
        return std::ranges::find(vec, id) != vec.end();
    }
    return false;
}

bool SelectionSystem::is_selected(const Entity entity, const Entity scope)
{
    return is_selected(entity.get_name(), scope);
}

bool SelectionSystem::has_selection(Entity scope)
{
    return scope.has_component<SelectionComponent>() &&
        !scope.get_component<SelectionComponent>().selections.empty();
}

StringId SelectionSystem::get_selected(Entity scope)
{
    if (has_selection(scope))
        return scope.get_component<SelectionComponent>().selections.front();
    return {};
}

StringId SelectionSystem::get_selection_by_index(const Entity scope, const size_t index)
{
    const auto selections = get_selections(scope);
    return selections[index];
}

const std::vector<StringId>& SelectionSystem::get_selections(Entity scope)
{
    const static std::vector<StringId> empty{};

    if (scope.has_component<SelectionComponent>())
        return scope.get_component<SelectionComponent>().selections;
    return empty;
}

size_t SelectionSystem::selection_count(Entity scope)
{
    if (scope.has_component<SelectionComponent>())
        return scope.get_component<SelectionComponent>().selections.size();
    return 0;
}

void SelectionSystem::deselect(const StringId& id, ecs::Registry& registry)
{
    std::vector<entt::entity> entities_to_remove;
    for (auto [entity_id, selected] : registry.get_raw_registry().view<SelectionComponent>().each())
    {
        auto& vec = selected.selections;
        std::erase(vec, id);
        if (vec.empty())
            entities_to_remove.push_back(entity_id);
    }

    for (auto& entity_id : entities_to_remove)
        registry.get_raw_registry().remove<SelectionComponent>(entity_id);

    LOGGER_TRACE("Deselected {} from all entities", id);
}

void SelectionSystem::deselect(const StringId id, Entity scope)
{
    if (scope.has_component<SelectionComponent>())
    {
        scope.patch_component<SelectionComponent>(
            [id](auto& comp)
            {
                std::erase(comp.selections, id);
            }
        );

        if (scope.get_component<SelectionComponent>().selections.empty())
            scope.remove_component<SelectionComponent>();
    }

    LOGGER_TRACE("Deselected {} from {}", id, scope.get_name());
}

void SelectionSystem::deselect_all(const Entity scope)
{
    if (scope.has_component<SelectionComponent>())
        scope.remove_component<SelectionComponent>();

    LOGGER_TRACE("Deselected all entities from {}", scope.get_name());
}

Entity SelectionSystem::selection_to_entity(const StringId& id, const Entity scope)
{
    for (auto&& [entity, tag] : scope.get_registry().view<NameComponent>().each())
    {
        if (tag.name == id)
            return Entity{entity, scope.get_registry()};
    }
    return null_entity;
}

Entity SelectionSystem::get_selected_entity(Entity scope)
{
    if (has_selection(scope))
    {
        auto id = scope.get_component<SelectionComponent>().selections.front();
        return selection_to_entity(id, scope);
    }
    return null_entity;
}
} // portal
