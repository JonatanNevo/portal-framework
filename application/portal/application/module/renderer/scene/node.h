//
// Created by Jonatan Nevo on 11/03/2025.
//

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "portal/application/module/renderer/scene/components/transform.h"


namespace portal::scene_graph
{
class Component;

/// @brief A leaf of the tree structure which can have children and a single parent.
class Node
{
public:
    Node(size_t id, const std::string& name);
    virtual ~Node() = default;

    void add_child(Node& child);
    void set_parent(Node& parent);
    void set_component(Component& component);

    size_t get_id() const;
    const std::string& get_name() const;
    const Transform& get_transform() const;
    Node* get_parent() const;
    const std::vector<Node*>& get_children() const;

    Component& get_component(const std::type_index index) const;

    template <class T>
    T& get_component() const
    {
        return dynamic_cast<T&>(get_component(typeid(T)));
    }

    bool has_component(const std::type_index index) const;

    template <class T>
    bool has_component() const
    {
        return has_component(typeid(T));
    }

private:
    size_t id;
    std::string name;
    Transform transform;
    Node* parent = nullptr;
    std::vector<Node*> children;
    std::unordered_map<std::type_index, Component*> components;
};
} // portal
