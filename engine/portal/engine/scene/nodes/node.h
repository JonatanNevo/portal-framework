//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <span>
#include <vector>

#include <portal/core/glm.h>

#include "portal/engine/reference.h"
#include "portal/core/strings/string_id.h"

namespace portal {
struct FrameContext;
}

namespace portal::scene
{
class Node
{
public:
    virtual ~Node() = default;
    explicit Node(const StringId& id, const glm::mat4& local_transform);

    void add_child(const Reference<Node>& child);
    void set_parent(const Reference<Node>& new_parent);

    [[nodiscard]] std::optional<Reference<Node>> get_parent() const;
    [[nodiscard]] bool has_children() const;
    [[nodiscard]] const std::vector<Reference<Node>>& get_children() const;
    [[nodiscard]] const StringId& get_id() const;
    [[nodiscard]] const glm::mat4& get_world_transform() const;
    [[nodiscard]] const glm::mat4& get_local_transform() const;

    void refresh_transform(const glm::mat4& parent_matrix);
    virtual void draw(const glm::mat4& top_matrix, FrameContext& frame);

protected:
    StringId id;
    WeakReference<Node> parent;
    std::vector<Reference<Node>> children;

    glm::mat4 local_transform;
    glm::mat4 world_transform;
};
} // portal
