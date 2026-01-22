//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/ecs/entity.h"
#include "portal/engine/ecs/registry.h"
#include "portal/engine/resources/resources/resource.h"

namespace portal
{
class Scene final : public Resource
{
public:
    explicit Scene(const StringId& name, ecs::Registry& registry);

    [[nodiscard]] Entity get_scene_entity() const { return scene_entity; }
    [[nodiscard]] Entity get_main_camera_entity() const;

    [[nodiscard]] ecs::Registry& get_registry() const { return ecs_registry; }

    void set_viewport_bounds(const glm::uvec4& bounds) { viewport_bounds = bounds; }
    [[nodiscard]] glm::uvec4 get_viewport_bounds() const { return viewport_bounds; }

private:
    ecs::Registry& ecs_registry;
    glm::uvec4 viewport_bounds;
    Entity scene_entity;
};
} // portal
