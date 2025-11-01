//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "node.h"
#include "portal/engine/resources/resources/mesh.h"

namespace portal::scene
{

class MeshNode final : public Node
{
public:
    explicit MeshNode(const StringId& id, const ResourceReference<Mesh>& mesh)
        : Node(id),
          mesh(mesh) {}

    void draw(const glm::mat4& top_matrix, DrawContext& context) override;

    ResourceReference<Mesh> mesh;
};
}
