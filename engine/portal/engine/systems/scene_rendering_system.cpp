//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_rendering_system.h"

#include "portal/engine/components/camera.h"
#include "portal/engine/components/light_components.h"
#include "portal/engine/renderer/rendering_context.h"
#include "portal/engine/scene/scene.h"
#include "portal/engine/scene/scene_context.h"

namespace portal
{

static auto logger = Log::get_logger("SceneRenderingSystem");

void SceneRenderingSystem::connect(ecs::Registry&, entt::dispatcher&) {}
void SceneRenderingSystem::disconnect(ecs::Registry&, entt::dispatcher&) {}

void SceneRenderingSystem::execute(FrameContext& frame, ecs::Registry& registry)
{
    update_global_descriptors(frame, registry);
    update_lights(frame, registry);
    add_static_mesh_to_context(frame, registry);
}

void SceneRenderingSystem::update_global_descriptors(FrameContext& frame, ecs::Registry& registry)
{
    auto* rendering_context = std::any_cast<renderer::FrameRenderingContext>(&frame.rendering_context);
    const auto scene = std::any_cast<SceneContext>(&frame.scene_context)->active_scene;

    // Camera information
    {
        const auto main_camera_group = registry.group<MainCameraTag, CameraComponent>();
        PORTAL_ASSERT(main_camera_group.size() == 1, "There should be exactly one camera tagged with MainCameraTag");

        auto camera_entity = registry.entity_from_id(main_camera_group.front());

        auto& camera = camera_entity.get_component<CameraComponent>();
        camera.set_viewport_bounds(scene->get_viewport_bounds());

        const glm::mat4 view = camera.view;
        glm::mat4 projection = camera.projection;

        // invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
        projection[1][1] *= -1;

        const auto view_projection = projection * view;

        rendering_context->scene_data.camera.view = view;
        rendering_context->scene_data.camera.proj = projection;
        rendering_context->scene_data.camera.view_proj = view_projection;;
        rendering_context->scene_data.camera.inverse_view = camera.inverse_view;
        rendering_context->scene_data.camera.inverse_proj = camera.inverse_projection;
        rendering_context->scene_data.camera.inverse_view_proj = camera.inverse_view * camera.inverse_projection;

        // TODO: set only on resize
        auto bounds = scene->get_viewport_bounds();
        auto width = static_cast<float>(bounds.z - bounds.x);
        auto height = static_cast<float>(bounds.w - bounds.y);

        rendering_context->scene_data.screen_data.full_resolution = glm::vec2(width, height);
        rendering_context->scene_data.screen_data.half_resolution = glm::vec2(width / 2.f, height / 2.f);
        rendering_context->scene_data.screen_data.inv_full_resolution = glm::vec2(1.f / width, 1.f / height);
        rendering_context->scene_data.screen_data.inv_half_resolution = glm::vec2(1.f / (width / 2.f), 1.f / (height / 2.f));
    }
}

void SceneRenderingSystem::update_lights(FrameContext& frame, const ecs::Registry& registry)
{
    // TODO: add dirty system, most lights wont change every frame.

    PORTAL_PROF_ZONE("Render Lights");
    auto* rendering_context = std::any_cast<renderer::FrameRenderingContext>(&frame.rendering_context);

    bool found_directional_light = false;
    for (auto entity : registry.view<DirectionalLightComponent>())
    {
        if (found_directional_light)
        {
            LOGGER_ERROR("Multiple directional lights are not supported");
            break;
        }

        const auto& directional_light = entity.get_component<DirectionalLightComponent>();

        rendering_context->scene_lights.directional_light.directional_light.direction = directional_light.direction;
        rendering_context->scene_lights.directional_light.directional_light.radiance = directional_light.radiance;
        rendering_context->scene_lights.directional_light.directional_light.multiplier = directional_light.intensity;
        rendering_context->scene_lights.directional_light.directional_light.shadow_amount = 1.f;

        rendering_context->scene_lights.directional_light.environment_map_intensity = 1.f; // TODO: get from scene configuration

        found_directional_light = true;
    }

    bool found_skylight = false;
    for (auto entity: registry.view<SkylightComponent>())
    {
        if (found_skylight)
        {
            LOGGER_ERROR("Multiple skylights are not supported");
            break;
        }

        // const auto& skylight = entity.get_component<SkylightComponent>();

        // TODO: skylight.intensity should map to the `environment_map_intensity` of the directional lights

        found_skylight = true;
    }

    if (!found_skylight)
    {
        // Setup black texture for skylight
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
