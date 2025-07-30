//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "gltf_scene.h"

#include <tracy/Tracy.hpp>
#include "portal/engine/renderer/scene/scene_node.h"

namespace portal
{
GLTFScene::~GLTFScene()
{
    clear_all();
}

void GLTFScene::draw(const glm::mat4& top_matrix, DrawContext& context)
{
    ZoneScoped;
    for (const auto& n: top_nodes)
    {
        n->draw(top_matrix, context);
    }
}

void GLTFScene::clear_all()
{
    material_data = nullptr;

    nodes.clear();
    top_nodes.clear();

    meshes.clear();
    images.clear();
    materials.clear();
    samplers.clear();

    descriptor_allocator.destroy_pools();
}
} // portal
