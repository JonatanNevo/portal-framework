//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_node.h"
#include "draw_context.h"
#include <tracy/Tracy.hpp>
#include "portal/engine/renderer/loader.h"

namespace portal
{

void SceneNode::refresh_transform(const glm::mat4& parent_matrix)
{
    ZoneScoped;
    world_transform = parent_matrix * local_transform;
    for (const auto& child : children)
    {
        child->refresh_transform(world_transform);
    }
}

void SceneNode::draw(const glm::mat4& top_matrix, DrawContext& context)
{
    ZoneScoped;
    for (const auto& child : children)
    {
        child->draw(top_matrix, context);
    }
}

void MeshNode::draw(const glm::mat4& top_matrix, DrawContext& context)
{
    ZoneScoped;
    const glm::mat4 node_matrix = top_matrix * world_transform;

    for (auto& [start_index, count, bounds, material] : mesh->surfaces)
    {
        RenderObject object{
            .index_count = count,
            .first_index = start_index,
            .index_buffer = &mesh->mesh_buffers.index_buffer,
            .material = &material->data,
            .bounds = bounds,
            .transform = node_matrix,
            .vertex_buffer_address = mesh->mesh_buffers.vertex_buffer_address,
        };

        if (material->data.pass_type == MaterialPass::Transparent)
            context.transparent_surfaces.push_back(object);
        else
            context.opaque_surfaces.push_back(object);
    }

    SceneNode::draw(top_matrix, context);
}

} // portal
