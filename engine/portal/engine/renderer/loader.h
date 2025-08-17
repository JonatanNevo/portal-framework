//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <fastgltf/core.hpp>

#include <portal/core/files/file_system.h>

#include "rendering_types.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/scene/materials/material.h"

namespace portal
{
struct GLTFScene;
class Renderer;
}

namespace portal::vulkan
{
struct GLTFMaterial
{
    MaterialInstance data;
};

struct GeoSurface
{
    uint32_t start_index;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers mesh_buffers;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

std::optional<std::shared_ptr<GLTFScene>> load_gltf(vk::raii::Device& device, std::filesystem::path path, Renderer* renderer);


}
