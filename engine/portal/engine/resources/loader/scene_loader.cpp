//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_loader.h"

#include "portal/core/buffer_stream.h"
#include "portal/core/variant.h"
#include "portal/engine/components/mesh.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/resources/mesh_geometry.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/scene/scene.h"
#include "portal/serialization/serialize.h"
#include "portal/serialization/serialize/binary_serialization.h"

namespace portal::resources
{

struct SerMeshComp
{
    StringId mesh_id;
    std::vector<StringId> materials;
};

void NodeDescription::archive(ArchiveObject& archive) const
{
    archive.add_property("name", name);
    archive.add_property("children", children);
    archive.add_property("parent", parent);

    auto* child = archive.create_child("components");
    for (auto& comp : components)
    {
        match(comp,
           [&](const TransformSceneComponent& transform)
           {
               child->add_property("transform", transform);
           },
           [&](const MeshSceneComponent& mesh)
           {
               child->add_property("mesh", mesh);
           }
           );
    }
}

NodeDescription NodeDescription::dearchive(ArchiveObject& archive)
{
    NodeDescription description;
    archive.get_property("name", description.name);
    archive.get_property("children", description.children);
    archive.get_property("parent", description.parent);

    auto* components = archive.get_object("components");
    for (auto& [name, _] : *components)
    {
        if (name == "transform")
        {
            auto& variant = description.components.emplace_back(TransformSceneComponent());
            components->get_property("transform", std::get<TransformSceneComponent>(variant));
        }
        else if (name == "mesh")
        {
            auto& variant = description.components.emplace_back(MeshSceneComponent());
            components->get_property("mesh", std::get<MeshSceneComponent>(variant));
        }
    }

    return description;
}

void NodeDescription::serialize(Serializer& serializer) const
{
    serializer.add_value(name);
    serializer.add_value(children);
    serializer.add_value(parent);

    serializer.add_value<size_t>(components.size());
    for (auto component : components)
    {
        match(component,
            [&](const TransformSceneComponent& transform)
            {
                serializer.add_value("TransformSceneComponent");
                serializer.add_value(transform.transform);
            },
            [&](const MeshSceneComponent& mesh)
            {
                serializer.add_value("MeshSceneComponent");
                serializer.add_value(mesh.mesh_id);
                serializer.add_value(mesh.materials);
            }
            );
    }
}

NodeDescription NodeDescription::deserialize(Deserializer& deserializer)
{
    NodeDescription description;
    deserializer.get_value(description.name);
    deserializer.get_value(description.children);
    deserializer.get_value(description.parent);

    size_t component_count;
    deserializer.get_value(component_count);
    description.components.reserve(component_count);

    for (size_t i = 0; i < component_count; ++i)
    {
        std::string component_type;
        deserializer.get_value(component_type);

        if (component_type == "TransformSceneComponent")
        {
            auto& variant = description.components.emplace_back(TransformSceneComponent());
            auto& transform = std::get<TransformSceneComponent>(variant);

            deserializer.get_value<glm::mat4>(transform.transform);
        }

        else if (component_type == "MeshSceneComponent")
        {
            auto& variant = description.components.emplace_back(MeshSceneComponent());
            auto& mesh = std::get<MeshSceneComponent>(variant);

            deserializer.get_value(mesh.mesh_id);
            deserializer.get_value(mesh.materials);
        }
    }

    return description;
}

SceneLoader::SceneLoader(ResourceRegistry& registry) : ResourceLoader(registry)
{}

ResourceData SceneLoader::load(const SourceMetadata& meta, Reference<ResourceSource> source)
{
    const auto description = load_scene_description(meta, *source);
    const auto scene = make_reference<Scene>(meta.resource_id, registry.get_ecs_registry());

    load_scene_nodes(scene->get_scene_entity(), scene->get_registry(), description);
    return {scene, source, meta};
}

void SceneLoader::save(const ResourceData& resource_data)
{
    const auto scene = reference_cast<Scene>(resource_data.resource);
    const auto dirty = resource_data.dirty;

    if (dirty & ResourceDirtyBits::DataChange)
    {
        // TODO: save the scene to disk
    }
}

class NodeComponentVisitor
{
public:
    NodeComponentVisitor(
        const Entity entity,
        ResourceRegistry& registry
    )
        : registry(registry),
          entity(entity) {}

    void operator()(const TransformSceneComponent& transform_component)
    {
        entity.add_component<TransformComponent>(transform_component.transform);
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

        NodeComponentVisitor visitor(entity, registry);
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
        BufferStreamReader reader{data};

        SceneDescription description{};
        BinaryDeserializer deserializer{reader};
        deserializer.get_value(description);
        return description;
    }

    throw std::runtime_error("Unknown scene format");
}
}
