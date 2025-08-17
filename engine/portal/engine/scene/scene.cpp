//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene.h"

namespace portal
{
Scene::Scene(const StringId& id) : Resource(id)
{}

void Scene::copy_from(const Ref<Resource> resource)
{
    root_nodes = resource.as<Scene>()->root_nodes;
}

void Scene::add_root_node(const Ref<scene::Node>& node)
{
    root_nodes.push_back(node);
}

std::span<Ref<scene::Node>> Scene::get_root_nodes()
{
    return root_nodes;
}

void Scene::draw(const glm::mat4& top_matrix, scene::DrawContext& context)
{
    for (auto& n: root_nodes)
    {
        n->draw(top_matrix, context);
    }
}
} // portal
