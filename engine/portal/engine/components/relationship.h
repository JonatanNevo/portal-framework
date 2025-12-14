//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/engine/ecs/entity.h>

namespace portal
{
struct RelationshipComponent
{
    // Children
    size_t children;
    Entity first = null_entity;
    Entity prev = null_entity;
    Entity next = null_entity;

    // Parent
    Entity parent = null_entity;
};
}
