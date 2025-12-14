//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_loader.h"

#include "portal/engine/components/mesh.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/resources/mesh_geometry.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/scene/scene.h"

namespace portal::resources
{
SceneLoader::SceneLoader(ResourceRegistry& registry) : ResourceLoader(registry)
{}

Reference<Resource> SceneLoader::load(const SourceMetadata& meta, const ResourceSource& source)
{
    const auto description = load_scene_description(meta, source);
    const auto scene = make_reference<Scene>(meta.resource_id);
    registry.configure_ecs_registry(scene->get_registry());

    load_scene_nodes(scene->get_scene_entity(), scene->get_registry(), description);
    return scene;
}

class NodeComponentVisitor
{
public:
    NodeComponentVisitor(
        const NodeDescription& description,
        const Entity entity,
        ResourceRegistry& registry
    )
        : registry(registry),
          entity(entity),
          description(description) {}

    void operator()(const TransformSceneComponent& transform_component)
    {
        entity.patch_component<TransformComponent>(
            [transform_component](auto& comp)
            {
                comp.set_matrix(transform_component.transform);
            }
        );
    }

    void operator()(const MeshSceneComponent& mesh_component)
    {
        auto& [mesh_id, materials_ids] = mesh_component;
        auto mesh_geometry = registry.immediate_load<MeshGeometry>(mesh_id);
        PORTAL_ASSERT(mesh_geometry.get_state() == ResourceState::Loaded, "Failed to load mesh");

        std::vector<ResourceReference<renderer::Material>> materials;
        materials.reserve(materials_ids.size());
        for (const auto& material_id : materials_ids)
        {
            materials.emplace_back(registry.immediate_load<renderer::Material>(material_id));
            PORTAL_ASSERT(materials.back().get_state() == ResourceState::Loaded, "Failed to load material");
        }

        entity.add_component<StaticMeshComponent>(mesh_geometry, std::move(materials));
    }

private:
    ResourceRegistry& registry;
    Entity entity;
    const NodeDescription& description;
};

void SceneLoader::load_scene_nodes(Entity scene_entity, ecs::Registry& ecs_registry, SceneDescription description) const
{
    for (auto& node_description : description.nodes)
    {
        Entity entity;
        if (node_description.parent.has_value())
            entity = ecs_registry.find_or_create_child(
                ecs_registry.find_or_create_child(scene_entity, *node_description.parent),
                node_description.name
            );
        else
            entity = ecs_registry.find_or_create_child(scene_entity, node_description.name);

        for (auto& child : node_description.children)
        {
            auto child_entity = ecs_registry.find_or_create(child);
            child_entity.set_parent(entity);
        }

        NodeComponentVisitor visitor(node_description, entity, registry);
        for (auto& component : node_description.components)
        {
            std::visit(visitor, component);
        }
    }
}

SceneDescription SceneLoader::load_scene_description(const SourceMetadata& meta, const ResourceSource& source)
{
    if (meta.format == SourceFormat::Memory)
    {
        // TODO: Properly serialize scene description instead of loading vectors directly from memory
        auto data = source.load();
        return data.read<SceneDescription>();
    }

    throw std::runtime_error("Unknown scene format");
}
}

