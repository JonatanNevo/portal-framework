//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "register_component.h"
#include "portal/engine/ecs/entity.h"

namespace portal
{

struct SelectionComponent
{
    Entity selected_entity;
};
}
