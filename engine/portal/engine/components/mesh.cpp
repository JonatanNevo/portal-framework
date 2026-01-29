//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "mesh.h"

namespace portal
{
void StaticMeshComponent::archive(ArchiveObject& archive) const
{
    archive.add_property("mesh", mesh->get_id());
    archive.add_property(
        "materials",
        std::ranges::to<std::vector>(
            materials | std::views::transform(
                [](const auto& material)
                {
                    return material->get_id().string;
                }
            )
        )
    );
    archive.add_property("visible", visible);
}

StaticMeshComponent StaticMeshComponent::dearchive(ArchiveObject& archive)
{
    StaticMeshComponent comp;

    StringId mesh_id;
    std::vector<std::string> material_ids;
    archive.get_property("mesh", mesh_id);
    archive.get_property("materials", material_ids);
    archive.get_property("visible", comp.visible);

    comp.mesh = ResourceReference<MeshGeometry>(mesh_id);
    comp.materials = std::ranges::to<std::vector>(
        material_ids | std::views::transform(
            [](const auto& string)
            {
                return ResourceReference<renderer::Material>(STRING_ID(string));
            }
        )
    );
    return comp;
}

void StaticMeshComponent::serialize(Serializer& serializer) const
{
    serializer.add_value(mesh->get_id());
    serializer.add_value(
        std::ranges::to<std::vector>(materials | std::views::transform([](const auto& material) { return material->get_id(); }))
    );
    serializer.add_value(visible);
}

StaticMeshComponent StaticMeshComponent::deserialize(Deserializer& serializer)
{
    StaticMeshComponent comp;

    StringId mesh_id;
    std::vector<StringId> material_ids;
    serializer.get_value(mesh_id);
    serializer.get_value(material_ids);
    serializer.get_value(comp.visible);

    comp.mesh = ResourceReference<MeshGeometry>(mesh_id);
    comp.materials = std::ranges::to<std::vector>(
        material_ids | std::views::transform([](const auto& id) { return ResourceReference<renderer::Material>(id); })
    );
    return comp;
}


void StaticMeshComponent::post_serialization(Entity, ResourceRegistry& reg)
{
    mesh = reg.immediate_load<MeshGeometry>(mesh.get_resource_id());
    materials = std::ranges::to<std::vector>(
        materials | std::views::transform(
            [&reg](const auto& material)
            {
                return reg.immediate_load<renderer::Material>(material.get_resource_id());
            }
        )
    );
}

std::vector<StringId> StaticMeshComponent::get_dependencies()
{
    auto deps = std::ranges::to<std::vector>(materials | std::views::transform([](const auto& material) { return material.get_resource_id(); }));
    deps.push_back(mesh.get_resource_id());
    return deps;
}
}
