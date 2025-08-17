//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "mesh_node.h"

#include "portal/engine/scene/draw_context.h"

namespace portal::scene
{

void MeshNode::draw(const glm::mat4& top_matrix, DrawContext& context)
{
    const glm::mat4 node_matrix = top_matrix * world_transform;

    for (auto& [start_index, count, bounds, material] : mesh->surfaces)
    {
        RenderObject object{
            .index_count = count,
            .first_index = start_index,
            .index_buffer = mesh->mesh_data.index_buffer,
            .material = material,
            .bounds = bounds,
            .transform = node_matrix,
            .vertex_buffer_address = mesh->mesh_data.vertex_buffer_address,
        };

        context.render_objects.push_back(object);
    }

    Node::draw(top_matrix, context);
}

}
