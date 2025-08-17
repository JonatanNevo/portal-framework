//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "node.h"
#include "portal/core/reference.h"
#include "portal/engine/resources/resources/mesh.h"

namespace portal::scene
{

class MeshNode final : public Node
{
public:
    explicit MeshNode(const StringId& id)
        : Node(id),
          mesh(nullptr) {}

    void draw(const glm::mat4& top_matrix, DrawContext& context) override;

    Ref<Mesh> mesh;
};
}
