//
// Created by Jonatan Nevo on 11/03/2025.
//

#include "node.h"

namespace portal::scene_graph
{
Node::Node(const size_t id, const std::string& name):
    id(id),
    name(name),
    transform(*this)
{
    set_component(transform);
}

void Node::add_child(Node& child)
{
    children.push_back(&child);
}

size_t Node::get_id() const
{
    return id;
}

const std::string& Node::get_name() const
{
    return name;
}

const Transform& Node::get_transform() const
{
    return transform;
}

Node* Node::get_parent() const
{
    return parent;
}

const std::vector<Node*>& Node::get_children() const
{
    return children;
}

Component& Node::get_component(const std::type_index index) const
{
    return *components.at(index);
}

void Node::set_parent(Node& parent)
{
    this->parent = &parent;
    transform.invalidate_world_matrix();
}

void Node::set_component(Component& component)
{
    const auto it = components.find(component.get_type());
    if (it != components.end())
    {
        it->second = &component;
    }
    else
    {
        components[component.get_type()] = &component;
    }
}

bool Node::has_component(const std::type_index index) const
{
    return components.contains(index);
}
} // portal
