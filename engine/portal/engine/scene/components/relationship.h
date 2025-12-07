//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <entt/entt.hpp>

namespace portal
{
struct RelationshipComponent
{
    // Children
    size_t children;
    entt::entity first = entt::null;
    entt::entity prev = entt::null;
    entt::entity next = entt::null;

    // Parent
    entt::entity parent = entt::null;
};
}