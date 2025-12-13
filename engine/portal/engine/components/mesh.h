//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resource_reference.h"
#include "portal/engine/resources/resources/mesh_geometry.h"

namespace portal
{

struct StaticMeshComponent
{
    ResourceReference<MeshGeometry> mesh;
    std::vector<ResourceReference<renderer::Material>> materials;
    bool visible = true;
};

} // portal