//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene.h"

#include "portal/engine/components/base.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/relationship.h"
#include "portal/engine/renderer/rendering_context.h"

namespace portal
{
Scene::Scene(const StringId& name, ecs::Registry& registry) : Resource(name), ecs_registry(registry)
{
    scene_entity = registry.create_entity(name);
    scene_entity.add_component<RelationshipComponent>();
    scene_entity.add_component<SceneTag>();
}

Entity Scene::get_main_camera_entity() const
{
    return ecs_registry.view<CameraComponent, MainCameraTag>().front();
}
} // portal
