//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "mesh_node.h"

#include "portal/engine/scene/draw_context.h"

namespace portal::scene
{

MeshNode::MeshNode(
    const StringId& id,
    const glm::mat4& local_transform,
    const ResourceReference<MeshGeometry>& mesh,
    const std::vector<ResourceReference<renderer::Material>>& materials
    ) : Node(id, local_transform),
        mesh(mesh),
        materials(materials)
{
    PORTAL_ASSERT(materials.size() == mesh->get_submeshes().size(), "Invalid number of materials");
}

void MeshNode::draw(const glm::mat4& top_matrix, DrawContext& context)
{
    const glm::mat4 node_matrix = top_matrix * world_transform;

    int i = 0;
    for (auto& [start_index, count, bounds] : mesh->get_submeshes())
    {
        RenderObject object{
            .index_count = count,
            .first_index = start_index,
            .index_buffer = mesh->get_index_buffer(),
            .material = materials[i++].underlying(),
            .bounds = bounds,
            .transform = node_matrix,
            .vertex_buffer_address = mesh->get_vertex_buffer_address(),
        };

        context.render_objects.push_back(object);
    }

    Node::draw(top_matrix, context);
}

}
