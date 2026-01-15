//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/ecs/entity.h"

namespace portal
{

class SelectionSystem
{
public:
    static void select(Entity entity, Entity scope);
    static bool is_selected(Entity entity);
    static bool is_selected(Entity entity, Entity scope);
    static bool has_selection(Entity scope);
    static Entity get_selected_entity(Entity scope);
    static void deselect(Entity entity);
    static void deselect(Entity entity, Entity scope);
};

} // portal