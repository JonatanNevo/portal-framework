//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <span>

#include "portal/engine/reference.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/scene/nodes/node.h"

namespace portal
{

class Scene final : public Resource
{
public:
    explicit Scene(const StringId& id);

    Scene(const Scene& other);

    void add_root_node(const Reference<scene::Node>& node);
    std::span<Reference<scene::Node>> get_root_nodes();

    void draw(const glm::mat4& top_matrix, scene::DrawContext& context);
private:
    std::vector<Reference<scene::Node>> root_nodes;
};

} // portal