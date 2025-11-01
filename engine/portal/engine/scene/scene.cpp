//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene.h"

#include "portal/engine/reference.h"

namespace portal
{

Scene::Scene(const StringId& id) : Resource(id)
{}

Scene::Scene(const Scene& other): Resource(other), root_nodes(other.root_nodes)
{

}

void Scene::add_root_node(const Reference<scene::Node>& node)
{
    root_nodes.push_back(node);
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
} // portal
