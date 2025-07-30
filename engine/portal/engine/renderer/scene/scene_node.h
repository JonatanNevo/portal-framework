//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>
#include <vector>

#include "renderable.h"
#include "portal/engine/renderer/loader.h"

namespace portal
{

struct SceneNode: public Renderable
{
    std::weak_ptr<SceneNode> parent;
    std::vector<std::shared_ptr<SceneNode>> children;

    glm::mat4 local_transform;
    glm::mat4 world_transform;

    void refresh_transform(const glm::mat4& parent_matrix);
    void draw(const glm::mat4& top_matrix, DrawContext& context) override;
};

struct MeshNode: public SceneNode
{
    std::shared_ptr<vulkan::MeshAsset> mesh;

    void draw(const glm::mat4& top_matrix, DrawContext& context) override;
};

} // portal
