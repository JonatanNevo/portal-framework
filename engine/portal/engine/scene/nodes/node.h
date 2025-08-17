//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vector>

#include <portal/core/glm.h>

#include "portal/core/reference.h"
#include "portal/engine/strings/string_id.h"

namespace portal::scene {
struct DrawContext;
}

namespace portal::scene
{

struct Node: RefCounted
{
    explicit Node(StringId id);

    StringId id;
    WeakRef<Node> parent;
    std::vector<Ref<Node>> children;

    glm::mat4 local_transform;
    glm::mat4 world_transform;

    void refresh_transform(const glm::mat4& parent_matrix);
    virtual void draw(const glm::mat4& top_matrix, DrawContext& context);
};

} // portal
