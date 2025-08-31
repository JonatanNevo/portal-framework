//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "node.h"

#include "portal/core/debug/profile.h"

namespace portal::scene
{
Node::Node(const StringId& id): id(id) {}

void Node::refresh_transform(const glm::mat4& parent_matrix)
{
    PORTAL_PROF_ZONE();
    world_transform = parent_matrix * local_transform;
    for (auto& child : children)
    {
        child->refresh_transform(world_transform);
    }
}

void Node::draw(const glm::mat4& top_matrix, DrawContext& context)
{
    for (auto& child : children)
    {
        child->draw(top_matrix, context);
    }
}

} // portal