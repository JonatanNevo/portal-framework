//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene.h"

#include "components/base.h"
#include "components/camera.h"
#include "components/mesh.h"
#include "components/relationship.h"
#include "components/transform.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/rendering_context.h"

namespace portal
{
static auto logger = Log::get_logger("Scene");

ng::Scene::Scene(StringId name)
{
    scene_entity = registry.create();
    registry.emplace<TagComponent>(scene_entity, name);

    entt::sigh_helper{registry}
        .with<TransformComponent>()
        .on_construct<&entt::registry::emplace_or_replace<Dirty>>()
        .on_update<&entt::registry::emplace_or_replace<Dirty>>()
        .on_destroy<&entt::registry::emplace_or_replace<Dirty>>();

    // Creating ground initially for performance considerations
    registry.group<Dirty, TransformComponent>();
}

ng::Scene::~Scene()
{
    for (const auto entity : registry.view<entt::entity>() | std::views::transform([&](const auto ent) { return Entity{ent, registry}; }))
    {
        destroy_entity(entity, true, false);
    }

    registry.clear();
}

void ng::Scene::update(float dt)
{
    PORTAL_PROF_ZONE();

    dt = dt * time_scale;

    // Update dirty Transforms
    const auto transforms_to_update = registry.group<Dirty, TransformComponent>();
    transforms_to_update.sort(
        [this](const entt::entity lhs, const entt::entity rhs)
        {
            const auto& left_rel = registry.get<RelationshipComponent>(lhs);
            const auto& right_rel = registry.get<RelationshipComponent>(rhs);

            return right_rel.parent == lhs
                || left_rel.next == rhs
                || (!(left_rel.parent == rhs || right_rel.next == lhs)
                    && (left_rel.parent < right_rel.parent || (left_rel.parent == right_rel.parent && &left_rel < &right_rel)));
        }
    );

    for (auto&& [entity, transform] : transforms_to_update.each())
    {
        const auto& relationship = registry.get<RelationshipComponent>(entity);

        auto parent_matrix = glm::mat4(1.0f);
        if (relationship.parent != entt::null)
        {
            auto& parent_transform = registry.get<TransformComponent>(relationship.parent);
            parent_matrix = parent_transform.get_world_matrix();
        }
        transform.calculate_world_matrix(parent_matrix);
    }

    registry.clear<Dirty>();

    // TODO: call all relevant systems, maybe as jobs on the scheduler?
}

void ng::Scene::render(FrameContext& frame)
{
    PORTAL_PROF_ZONE();
    auto camera_entity = get_main_camera_entity();
    if (!camera_entity)
    {
        LOGGER_WARN("Scene has no active camera");
        return;
    }

    auto* rendering_context = std::any_cast<renderer::FrameRenderingContext>(&frame.rendering_context);


    glm::mat4 camera_view_matrix = glm::inverse(get_world_transform(camera_entity));
    auto camera = camera_entity.get_component<CameraComponent>();
    // TODO: give viewport bounds (through frame context maybe?)
    camera.set_viewport_bounds(rendering_context->viewport_bounds);

    // TODO: move to dedicated system (probably renderer because these are gpu data)
    {
        auto projection = camera.camera.get_projection();
        // invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
        projection[1][1] *= -1;

        const auto view_projection = projection * camera_view_matrix;
        const auto inverse_view = glm::inverse(camera_view_matrix);
        const auto inverse_projection = glm::inverse(projection);

        rendering_context->camera_data.view = camera_view_matrix;
        rendering_context->camera_data.proj = projection;
        rendering_context->camera_data.view_proj = view_projection;
        rendering_context->camera_data.inverse_view = inverse_view;
        rendering_context->camera_data.inverse_proj = inverse_projection;
        rendering_context->camera_data.inverse_view_proj = inverse_view * inverse_projection;

        rendering_context->scene_data.view = camera_view_matrix;
        rendering_context->scene_data.proj = projection;
        rendering_context->scene_data.view_proj = view_projection;
        rendering_context->scene_data.ambient_color = glm::vec4(.1f);
        rendering_context->scene_data.sunlight_color = glm::vec4(1.f);
        rendering_context->scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);
    }

    {
        PORTAL_PROF_ZONE("Render Static Mesh");
        for (auto entity : get_all_entities_with<StaticMeshComponent>())
        {
            auto& static_mesh = entity.get_component<StaticMeshComponent>();

            if (!static_mesh.visible)
                continue;

            if (static_mesh.mesh.is_valid())
            {
                const auto world_matrix = get_world_transform(entity);

                // TODO: move this to the relevant system
                int i = 0;
                for (auto& [start_index, count, bounds] : static_mesh.mesh->get_submeshes())
                {
                    renderer::RenderObject object{
                        .index_count = count,
                        .first_index = start_index,
                        .index_buffer = static_mesh.mesh->get_index_buffer(),
                        .material = static_mesh.materials[i++].underlying(),
                        .bounds = bounds,
                        .transform = world_matrix,
                        .vertex_buffer_address = static_mesh.mesh->get_vertex_buffer_address(),
                    };

                    rendering_context->render_objects.emplace_back(object);
                }
            }
        }
    }
}

Entity ng::Scene::create_entity(const StringId& name)
{
    return create_child_entity({entt::handle{}}, name);
}

Entity ng::Scene::create_child_entity(const Entity parent, const StringId& name)
{
    PORTAL_PROF_ZONE();

    auto entity = Entity{registry.create(), registry};
    populate_entity(entity, name, true);

    if (parent)
        entity.set_parent(parent);

    return entity;
}

void ng::Scene::destroy_entity(const Entity entity, const bool exclude_children, const bool first)
{
    PORTAL_PROF_ZONE();

    if (!entity)
        return;

    // Destroy child entities (if relevant) before we start destroying this entity
    if (!exclude_children)
    {
        for (const auto child : entity.children())
        {
            destroy_entity(child, exclude_children, first);
        }
    }

    if (first)
    {
        if (auto parent = entity.get_parent(); parent)
        {
            parent.remove_child(entity);
        }
    }

    // TODO: destroy components with custom deleter callback to ensure order
    registry.destroy(entity);

    sort_entities();
}

Entity ng::Scene::get_main_camera_entity() const
{
    PORTAL_PROF_ZONE();

    const auto view = registry.view<CameraComponent>();
    for (auto&& [entity, camera] : view.each())
    {
        if (camera.primary)
        {
            PORTAL_ASSERT(camera.orthographic_size || camera.perspective_vertical_fov, "Camera not initialized");
            return Entity{entity, const_cast<entt::registry&>(registry)};
        }
    }

    return Entity{entt::null, const_cast<entt::registry&>(registry)};
}

glm::mat4 ng::Scene::get_world_transform(Entity entity)
{
    const auto& transform = entity.get_component<TransformComponent>();
    return transform.get_world_matrix();
}

void ng::Scene::populate_entity(Entity& entity, StringId name, bool should_sort)
{
    entity.add_component<TransformComponent>();
    if (name != INVALID_STRING_ID)
        entity.add_component<TagComponent>(name);

    entity.add_component<RelationshipComponent>();

    if (should_sort)
        sort_entities();
}

void ng::Scene::sort_entities()
{
    // We sort the components based on the parents to enforce the parent first iteration order for transformation matrix calculation.
    // Consider opting to use 'Dirty' tag approach instead, see: https://skypjack.github.io/2019-08-20-ecs-baf-part-4-insights/
    registry.sort<RelationshipComponent>(
        [&](const entt::entity lhs, const entt::entity rhs)
        {
            const auto& left_rel = registry.get<RelationshipComponent>(lhs);
            const auto& right_rel = registry.get<RelationshipComponent>(rhs);

            return right_rel.parent == lhs
                || left_rel.next == rhs
                || (!(left_rel.parent == rhs || right_rel.next == lhs)
                    && (left_rel.parent < right_rel.parent || (left_rel.parent == right_rel.parent && &left_rel < &right_rel)));
        }
    );
}

Scene::Scene(const StringId& id, const std::vector<Reference<scene::Node>>& root_nodes) : Resource(id), root_nodes(root_nodes)
{
}

Scene::Scene(const Scene& other) : Resource(other), root_nodes(other.root_nodes)
{
}

std::span<Reference<scene::Node>> Scene::get_root_nodes()
{
    return root_nodes;
}

void Scene::draw(const glm::mat4& top_matrix, FrameContext& frame)
{
    for (auto& n : root_nodes)
    {
        n->draw(top_matrix, frame);
    }
}
} // portal
