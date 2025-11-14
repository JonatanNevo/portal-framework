//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_loader.h"

#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/scene/scene.h"
#include "portal/engine/scene/nodes/mesh_node.h"
#include "portal/engine/scene/nodes/node.h"

namespace portal::resources
{

SceneLoader::SceneLoader(ResourceRegistry& registry) : ResourceLoader(registry)
{}

Reference<Resource> SceneLoader::load(const SourceMetadata& meta, const ResourceSource& source)
{
    auto description = load_scene_description(meta, source);
    auto nodes = load_scene_nodes(description);

    // Populate child - parent relationship
    // TODO: this can probably be done through the initial `load_scene_nodes` somehow
    for (auto& node_desc: description.nodes)
    {
        const auto& node = nodes[node_desc.name];
        for (auto& child_id: node_desc.children)
        {
            node->add_child(nodes[child_id]);
        }

        if (node_desc.parent.has_value())
        {
            node->set_parent(nodes[node_desc.parent.value()]);
        }
    }

    std::vector<Reference<scene::Node>> root_nodes;
    for (const auto& scene_node_index : description.scene_nodes_ids)
    {
        auto& node_desc = description.nodes[scene_node_index];
        auto& node = nodes[node_desc.name];

        auto parent = node->get_parent();
        if (!parent.has_value())
        {
            root_nodes.push_back(node);
            node->refresh_transform(glm::mat4{1.f});
        }
    }

    return make_reference<Scene>(meta.resource_id, std::move(root_nodes));
}

class NodeComponentVisitor
{
public:
    NodeComponentVisitor(const NodeDescription& description, ResourceRegistry& registry) : registry(registry), description(description) {}

    void operator()(const TransformComponent& transform_component)
    {
        transform = transform_component;
    }

    void operator()(const MeshComponent& mesh_component)
    {
        mesh = mesh_component;
    }

    Reference<scene::Node> create_node() const
    {
        auto transform_value = transform.value_or(TransformComponent{});

        if (mesh.has_value())
        {
            auto& [mesh_id, materials_ids] = mesh.value();
            auto mesh_node = registry.immediate_load<MeshGeometry>(mesh_id);
            PORTAL_ASSERT(mesh_node.get_state() == ResourceState::Loaded, "Failed to load mesh");

            std::vector<ResourceReference<renderer::Material>> materials;
            materials.reserve(materials_ids.size());
            for (const auto& material_id : materials_ids)
            {
                materials.emplace_back(registry.immediate_load<renderer::Material>(material_id));
                PORTAL_ASSERT(materials.back().get_state() == ResourceState::Loaded, "Failed to load material");
            }

            return make_reference<scene::MeshNode>(description.name, transform_value.transform, mesh_node, std::move(materials));
        }

        return make_reference<scene::Node>(description.name, transform_value.transform);
    }

private:
    ResourceRegistry& registry;
    const NodeDescription& description;

    // TODO: change node class layout to components instead of inheritance and improve this visitor
    std::optional<TransformComponent> transform;
    std::optional<MeshComponent> mesh;
};


std::unordered_map<StringId, Reference<scene::Node>> SceneLoader::load_scene_nodes(SceneDescription description) const
{
    std::unordered_map<StringId, Reference<scene::Node>> nodes;
    for (auto& node_description : description.nodes)
    {
        NodeComponentVisitor visitor(node_description, registry);
        for (auto& component : node_description.components)
        {
            std::visit(visitor, component);
        }

        auto node = visitor.create_node();
        nodes[node_description.name] = std::move(node);
    }

    return nodes;
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

