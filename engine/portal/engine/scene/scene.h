//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <span>

#include "entity.h"
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
        ~Scene();

        void update(float dt);
        void render(FrameContext& frame);

        Entity create_entity(const StringId& name = INVALID_STRING_ID);
        Entity create_child_entity(Entity parent, const StringId& name = INVALID_STRING_ID);

        void destroy_entity(Entity entity, bool exclude_children = false, bool first = true);

        [[nodiscard]] Entity get_main_camera_entity() const;

        [[nodiscard]] static glm::mat4 get_world_transform(Entity entity);

        template <typename... T>
        auto get_all_entities_with()
        {
            return get_all_entities_with_internal<T...>() | std::views::transform([this](const auto entity) { return Entity{entity, registry}; });
        }

    private:
        template <typename... T>
        auto get_all_entities_with_internal()
        {
            return registry.view<T...>();
        }

        void populate_entity(Entity& entity, StringId name, bool should_sort);
        void sort_entities();

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
