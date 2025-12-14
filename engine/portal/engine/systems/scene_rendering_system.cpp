//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_rendering_system.h"

#include "portal/engine/components/camera.h"
#include "portal/engine/renderer/rendering_context.h"

namespace portal
{
void SceneRenderingSystem::execute(FrameContext& frame, ecs::Registry& registry)
{
    update_global_descriptors(frame, registry);
    add_static_mesh_to_context(frame, registry);
}

void SceneRenderingSystem::update_global_descriptors(FrameContext& frame, ecs::Registry& registry)
{
    auto* rendering_context = std::any_cast<renderer::FrameRenderingContext>(&frame.rendering_context);

    // Camera information
    {
        const auto main_camera_group = registry.group<MainCameraTag, CameraComponent>();
        PORTAL_ASSERT(main_camera_group.size() == 1, "There should be exactly one camera tagged with MainCameraTag");

        auto camera_entity = registry.entity_from_id(main_camera_group.front());

        auto camera = camera_entity.get_component<CameraComponent>();
        camera.set_viewport_bounds(rendering_context->viewport_bounds);

        const glm::mat4 view = camera.view;
        glm::mat4 projection = camera.projection;

        // invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
        projection[1][1] *= -1;

        const auto view_projection = projection * view;

        rendering_context->camera_data.view = view;
        rendering_context->camera_data.proj = projection;
        rendering_context->camera_data.view_proj = view_projection;
        rendering_context->camera_data.inverse_view = camera.inverse_view;
        rendering_context->camera_data.inverse_proj = camera.inverse_projection;
        rendering_context->camera_data.inverse_view_proj = camera.inverse_view * camera.inverse_projection;

        rendering_context->scene_data.view = view;
        rendering_context->scene_data.proj = projection;
        rendering_context->scene_data.view_proj = view_projection;
        rendering_context->scene_data.ambient_color = glm::vec4(.1f);
        rendering_context->scene_data.sunlight_color = glm::vec4(1.f);
        rendering_context->scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);
    }

}

void SceneRenderingSystem::add_static_mesh_to_context(FrameContext& frame, ecs::Registry& registry)
{
    auto* rendering_context = std::any_cast<renderer::FrameRenderingContext>(&frame.rendering_context);

    PORTAL_PROF_ZONE("Render Static Mesh");
    for (auto&& [entity, static_mesh, transform] : group(registry).each())
    {
        if (!static_mesh.visible)
            continue;

        if (static_mesh.mesh.is_valid())
        {
            const auto world_matrix = transform.get_world_matrix();

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
} // portal
