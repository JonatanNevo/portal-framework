//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "node.h"

#include <ranges>

#include "portal/core/debug/profile.h"

namespace portal::scene
{

Node::Node(const StringId& id, const glm::mat4& local_transform) : id(id), local_transform(local_transform), world_transform(local_transform)
{}

void Node::add_child(const Reference<Node>& child)
{
    children.emplace_back(child);
}

void Node::set_parent(const Reference<Node>& new_parent)
{
    parent = new_parent;
}

std::optional<Reference<Node>> Node::get_parent() const
{
    if (parent.expired() || parent.lock() == nullptr)
    {
        return std::nullopt;
    }

    return parent.lock();
}

bool Node::has_children() const
{
    return !children.empty();
}

const std::vector<Reference<Node>>& Node::get_children() const
{
    return children;
}

const StringId& Node::get_id() const
{
    return id;
}

const glm::mat4& Node::get_world_transform() const
{
    return world_transform;
}

const glm::mat4& Node::get_local_transform() const
{
    return local_transform;
}

void Node::refresh_transform(const glm::mat4& parent_matrix)
{
    world_transform = parent_matrix * local_transform;
    for (const auto& child : children)
    {
        child->refresh_transform(world_transform);
    }
}

void Node::draw(const glm::mat4& top_matrix, renderer::DrawContext& context)
{
    for (const auto& child : children)
    {
        child->draw(top_matrix, context);
    }
}

} // portal
