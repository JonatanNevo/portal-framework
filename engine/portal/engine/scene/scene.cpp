//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene.h"

#include "portal/engine/components/base.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/mesh.h"
#include "portal/engine/components/relationship.h"
#include "portal/engine/components/transform.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/rendering_context.h"

namespace portal
{
static auto logger = Log::get_logger("Scene");

ng::Scene::Scene(StringId name)
{
    scene_entity = registry.create();
    registry.emplace<NameComponent>(scene_entity, name);
}

void ng::Scene::update(float dt)
{
    PORTAL_PROF_ZONE();

    dt = dt * time_scale;

    // TODO: call all relevant systems, maybe as jobs on the scheduler?
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
