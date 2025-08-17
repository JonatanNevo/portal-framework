//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/allocated_buffer.h"
#include "portal/engine/resources/resources/material.h"
#include "portal/engine/resources/resources/resource.h"

namespace portal
{
namespace scene {
    class MeshNode;
}

namespace resources
{
    struct Vertex
    {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 color;
    };

    // TODO: move this to some other class?
    struct MeshData
    {
        std::shared_ptr<vulkan::AllocatedBuffer> index_buffer;
        std::shared_ptr<vulkan::AllocatedBuffer> vertex_buffer;
        vk::DeviceAddress vertex_buffer_address{};

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        MeshData& operator=(std::nullptr_t);
    };

    struct Bounds
    {
        glm::vec3 origin{};
        float sphere_radius{};
        glm::vec3 extents{};
    };

    struct Surface
    {
        uint32_t start_index{};
        uint32_t count{};
        Bounds bounds{};
        WeakRef<Material> material{};
    };
}

class Mesh final: public Resource
{
public:
    explicit Mesh(const StringId& id): Resource(id) {}
    void copy_from(Ref<Resource> other) override;;

public:
    friend class resources::GltfLoader;
    friend class scene::MeshNode;

    std::vector<resources::Surface> surfaces;
    resources::MeshData mesh_data;
};

} // portal