//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <span>

#include "llvm/ADT/DenseMap.h"
#include "portal/engine/reference.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/scene/nodes/node.h"

namespace portal
{

class Scene final : public Resource
{
public:
    Scene(const StringId& id, const std::vector<Reference<scene::Node>>& root_nodes);
    Scene(const Scene& other);

    std::span<Reference<scene::Node>> get_root_nodes();

    void draw(const glm::mat4& top_matrix, scene::DrawContext& context);
    void create_reference_map();

    [[nodiscard]] const llvm::DenseMap<StringId, WeakReference<scene::Node>>& get_nodes() const { return nodes; }

private:
    std::vector<Reference<scene::Node>> root_nodes;

    llvm::DenseMap<StringId, WeakReference<scene::Node>> nodes;
};

} // portal