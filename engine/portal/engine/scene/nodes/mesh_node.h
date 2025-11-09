//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "node.h"
#include "portal/engine/resources/resources/mesh_geometry.h"

namespace portal::scene
{

class MeshNode final : public Node
{
public:
    explicit MeshNode(
        const StringId& id,
        const glm::mat4& local_transform,
        const ResourceReference<MeshGeometry>& mesh,
        const std::vector<ResourceReference<renderer::Material>>& materials
        );

    void draw(const glm::mat4& top_matrix, DrawContext& context) override;

    [[nodiscard]] const ResourceReference<MeshGeometry>& get_mesh() const { return mesh; }
    [[nodiscard]] const std::vector<ResourceReference<renderer::Material>>& get_materials() const { return materials; }


private:
    ResourceReference<MeshGeometry> mesh;
    // One material per submesh
    std::vector<ResourceReference<renderer::Material>> materials;
};
}
