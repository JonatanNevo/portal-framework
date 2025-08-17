//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <span>

#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/scene/nodes/node.h"

namespace portal
{

class Scene final : public Resource
{
public:
    Scene() = default;
    explicit Scene(const StringId& id);

    void copy_from(Ref<Resource>) override;

    void add_root_node(const Ref<scene::Node>& node);
    std::span<Ref<scene::Node>> get_root_nodes();

    void draw(const glm::mat4& top_matrix, scene::DrawContext& context);
private:
    std::vector<Ref<scene::Node>> root_nodes;
};

} // portal