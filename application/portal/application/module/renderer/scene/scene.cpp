//
// Created by Jonatan Nevo on 11/03/2025.
//

#include "scene.h"

#include <queue>

#include "portal/application/module/renderer/scene/node.h"
#include "portal/application/module/renderer/scene/components/sub_mesh.h"

namespace portal::scene_graph
{
Scene::Scene(const std::string& name): name(name) {}

void Scene::set_name(const std::string& name)
{
    this->name = name;
}

const std::string& Scene::get_name() const
{
    return name;
}

void Scene::set_nodes(std::vector<std::unique_ptr<Node>>&& nodes)
{
    PORTAL_CORE_ASSERT(nodes.empty(), "Scene::set_nodes: nodes is empty");
    this->nodes = std::move(nodes);
}

void Scene::add_node(std::unique_ptr<Node>&& node)
{
    nodes.emplace_back(std::move(node));
}

void Scene::add_child(Node& child) const
{
    root->add_child(child);
}

std::unique_ptr<Component> Scene::get_model(const uint32_t index)
{
    auto meshes = std::move(components.at(typeid(SubMesh)));

    PORTAL_CORE_ASSERT(index < meshes.size(), "Scene::get_model: index out of bounds");
    return std::move(meshes[index]);
}

void Scene::add_component(std::unique_ptr<Component>&& component)
{
    if (component)
        components[component->get_type()].push_back(std::move(component));
}

void Scene::add_component(std::unique_ptr<Component>&& component, Node& node)
{
    node.set_component(*component);
    if (component)
        components[component->get_type()].push_back(std::move(component));
}

void Scene::set_components(const std::type_index& type_info, std::vector<std::unique_ptr<Component>>&& components)
{
    this->components[type_info] = std::move(components);
}

const std::vector<std::unique_ptr<Component>>& Scene::get_components(const std::type_index& type_info) const
{
    return components.at(type_info);
}

bool Scene::has_component(const std::type_index& type_info) const
{
    const auto it = components.find(type_info);
    return it != components.end() && !it->second.empty();
}

Node* Scene::find_node(const std::string& name) const
{
    for (auto& root_node : root->get_children())
    {
        std::queue<Node*> traverse_node{};
        traverse_node.push(root_node);

        while (!traverse_node.empty())
        {
            const auto node = traverse_node.front();
            traverse_node.pop();

            if (node->get_name() == name)
                return node;

            for (auto& child_node : node->get_children())
                traverse_node.push(child_node);
        }
    }

    return nullptr;
}

void Scene::set_root_node(Node& node)
{
    root = &node;
}

Node& Scene::get_root_node() const
{
    return *root;
}
} // portal
