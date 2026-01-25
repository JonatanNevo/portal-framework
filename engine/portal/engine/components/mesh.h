//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "register_component.h"
#include "portal/engine/resources/resource_reference.h"
#include "portal/engine/resources/resources/mesh_geometry.h"

namespace portal
{
struct StaticMeshComponent
{
    ResourceReference<MeshGeometry> mesh;
    std::vector<ResourceReference<renderer::Material>> materials;
    bool visible = true;

    void archive(ArchiveObject& archive) const;
    static StaticMeshComponent dearchive(ArchiveObject& archive);

    void serialize(Serializer& serializer) const;
    static StaticMeshComponent deserialize(Deserializer& serializer);

    void post_serialization(Entity entity, ResourceRegistry& reg);
    std::vector<StringId> get_dependencies();
};

REGISTER_COMPONENT(StaticMeshComponent);
} // portal
