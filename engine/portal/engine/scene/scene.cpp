//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene.h"

#include "portal/engine/reference.h"

namespace portal
{

Scene::Scene(const StringId& id, const std::vector<Reference<scene::Node>>& root_nodes): Resource(id), root_nodes(root_nodes)
{
    create_reference_map();
}

Scene::Scene(const Scene& other): Resource(other), root_nodes(other.root_nodes)
{

}

std::span<Reference<scene::Node>> Scene::get_root_nodes()
{
    return root_nodes;
}

void Scene::draw(const glm::mat4& top_matrix, scene::DrawContext& context)
{
    for (auto& n : root_nodes)
    {
        n->draw(top_matrix, context);
    }
}

void Scene::create_reference_map()
{
    nodes.clear();

    std::function<void(const Reference<scene::Node>&)> add_node_to_map;
    add_node_to_map = [&](const Reference<scene::Node>& node)
    {
        nodes[node->get_id()] = node;
        for (auto& child : node->get_children())
            add_node_to_map(child);
    };

    for (const auto& node: root_nodes)
    {
        add_node_to_map(node);
    }
}
} // portal
