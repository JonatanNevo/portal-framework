//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/resources/loader/loader.h"
#include "portal/engine/resources/resources/mesh_geometry.h"

namespace portal::resources
{
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<MeshGeometryData::Submesh> submeshes;
};

class MeshLoader final : public ResourceLoader
{
public:
    MeshLoader(ResourceRegistry& registry, const RendererContext& context);

    Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;

protected:
    MeshData load_mesh_data(const SourceMetadata& meta, const ResourceSource& source);

private:
    const RendererContext& context;
};
} // portal
