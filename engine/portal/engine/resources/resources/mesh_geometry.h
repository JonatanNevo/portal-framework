//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/engine/renderer/vulkan/allocated_buffer.h>

#include "portal/engine/reference.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/resources/resources/resource.h"

namespace portal
{

namespace scene
{
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

    struct Bounds
    {
        glm::vec3 origin{};
        float sphere_radius{};
        glm::vec3 extents{};
    };

    struct MeshGeometryData
    {
        std::shared_ptr<renderer::vulkan::AllocatedBuffer> index_buffer;
        std::shared_ptr<renderer::vulkan::AllocatedBuffer> vertex_buffer;
        vk::DeviceAddress vertex_buffer_address{};

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        struct Submesh
        {
            uint32_t start_index{};
            uint32_t count{};
            Bounds bounds{};
        };

        std::vector<Submesh> submeshes;
    };
}

class MeshGeometry final : public Resource
{
public:
    explicit MeshGeometry(const StringId& id, const resources::MeshGeometryData& geometry);

    [[nodiscard]] const std::shared_ptr<renderer::vulkan::AllocatedBuffer>& get_index_buffer() const;
    [[nodiscard]] const vk::DeviceAddress& get_vertex_buffer_address() const;

    [[nodiscard]] const resources::MeshGeometryData& get_geometry() const;
    [[nodiscard]] const std::vector<resources::MeshGeometryData::Submesh>& get_submeshes() const;

private:
    friend class scene::MeshNode;

    resources::MeshGeometryData geometry;
};

} // portal
