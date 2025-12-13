//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <span>

#include "portal/engine/ecs/entity.h"
#include "portal/engine/reference.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/scene/nodes/node.h"

namespace portal
{
namespace ng
{
    class Scene
    {
    public:
        explicit Scene(StringId name);

        void update(float dt);

    private:
        entt::entity scene_entity = entt::null;
        entt::registry registry;

        float time_scale = 1.0f;
    };
}

class Scene final : public Resource
{
public:
    Scene(const StringId& id, const std::vector<Reference<scene::Node>>& root_nodes);
    Scene(const Scene& other);

    std::span<Reference<scene::Node>> get_root_nodes();

    void draw(const glm::mat4& top_matrix, FrameContext& frame);

private:
    std::vector<Reference<scene::Node>> root_nodes;
};
} // portal
