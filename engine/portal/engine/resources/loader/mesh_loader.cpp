//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "mesh_loader.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "portal/core/buffer_stream.h"
#include "portal/engine/resources/source/resource_source.h"

namespace portal::resources
{
static auto logger = Log::get_logger("MeshLoader");

void calculate_tangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    std::vector<glm::vec3> bitangent_accum(vertices.size(), glm::vec3(0.f));

    // Zero out tangent accumulators
    for (auto& v : vertices)
        v.tangent = glm::vec4(0.f);

    // Accumulate per-triangle tangent and bitangent
    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        auto& v0 = vertices[indices[i]];
        auto& v1 = vertices[indices[i + 1]];
        auto& v2 = vertices[indices[i + 2]];

        const glm::vec3 edge1 = v1.position - v0.position;
        const glm::vec3 edge2 = v2.position - v0.position;

        const float du1 = v1.uv_x - v0.uv_x;
        const float dv1 = v1.uv_y - v0.uv_y;
        const float du2 = v2.uv_x - v0.uv_x;
        const float dv2 = v2.uv_y - v0.uv_y;

        const float det = du1 * dv2 - du2 * dv1;

        glm::vec3 tangent;
        glm::vec3 bitangent;

        if (std::abs(det) < 1e-8f)
        {
            // Degenerate UV triangle — pick a fallback tangent perpendicular to the normal
            const glm::vec3 n = glm::normalize(v0.normal + v1.normal + v2.normal);
            const glm::vec3 ref = std::abs(n.y) < 0.999f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(1.f, 0.f, 0.f);
            tangent = glm::normalize(glm::cross(n, ref));
            bitangent = glm::cross(n, tangent);
        }
        else
        {
            const float inv_det = 1.f / det;
            tangent = (edge1 * dv2 - edge2 * dv1) * inv_det;
            bitangent = (edge2 * du1 - edge1 * du2) * inv_det;
        }

        for (size_t j = 0; j < 3; ++j)
        {
            const uint32_t idx = indices[i + j];
            vertices[idx].tangent += glm::vec4(tangent, 0.f);
            bitangent_accum[idx] += bitangent;
        }
    }

    // Per-vertex finalization: Gram-Schmidt orthogonalization + handedness
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const glm::vec3 n = glm::normalize(vertices[i].normal);
        glm::vec3 t = glm::vec3(vertices[i].tangent);

        // Gram-Schmidt: remove the component of t along n
        t = t - n * glm::dot(n, t);
        const float len = glm::length(t);

        if (len < 1e-8f)
        {
            // Degenerate — pick a fallback tangent
            const glm::vec3 ref = std::abs(n.y) < 0.999f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(1.f, 0.f, 0.f);
            t = glm::normalize(glm::cross(n, ref));
        }
        else
        {
            t /= len;
        }

        // Compute handedness
        const float w = glm::dot(glm::cross(n, t), bitangent_accum[i]) < 0.f ? -1.f : 1.f;
        vertices[i].tangent = glm::vec4(t, w);
    }
}

MeshLoader::MeshLoader(ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context) : ResourceLoader(registry), context(context)
{}

ResourceData MeshLoader::load(const SourceMetadata& meta, Reference<ResourceSource> source)
{
    auto [vertices, indexes, submeshes] = load_mesh_data(meta, *source);

    MeshGeometryData geometry{
        .vertices = std::move(vertices),
        .indices = std::move(indexes),
        .submeshes = std::move(submeshes)
    };

    // Create mesh buffers
    const size_t vertex_buffer_size = geometry.vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = geometry.indices.size() * sizeof(uint32_t);

    renderer::vulkan::BufferBuilder vertex_builder{vertex_buffer_size};
    vertex_builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                  .with_usage(
                      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress
                      | vk::BufferUsageFlagBits::eTransferSrc
                  )
                  .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

    renderer::vulkan::BufferBuilder index_builder{index_buffer_size};
    index_builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                 .with_usage(
                     vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc
                 )
                 .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

    geometry.index_buffer = context.get_device().create_buffer_shared(index_builder);
    geometry.vertex_buffer = context.get_device().create_buffer_shared(vertex_builder);
    geometry.vertex_buffer_address = geometry.vertex_buffer->get_device_address();

    renderer::vulkan::BufferBuilder builder(vertex_buffer_size + index_buffer_size);
    builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
           .with_usage(vk::BufferUsageFlagBits::eTransferSrc)
           .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
           .with_debug_name("staging");
    auto staging_buffer = context.get_device().create_buffer(builder);
    auto offset = staging_buffer.update(geometry.vertices.data(), vertex_buffer_size, 0);
    offset += staging_buffer.update(geometry.indices.data(), index_buffer_size, offset);

    context.get_device().immediate_submit(
        [&](const vk::raii::CommandBuffer& command_buffer)
        {
            vk::BufferCopy vertex_copy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = vertex_buffer_size
            };
            command_buffer.copyBuffer(
                staging_buffer.get_handle(),
                geometry.vertex_buffer->get_handle(),
                {vertex_copy}
            );

            vk::BufferCopy index_copy{
                .srcOffset = vertex_buffer_size,
                .dstOffset = 0,
                .size = index_buffer_size
            };
            command_buffer.copyBuffer(
                staging_buffer.get_handle(),
                geometry.index_buffer->get_handle(),
                {index_copy}
            );
        }
    );

    return {make_reference<MeshGeometry>(meta.resource_id, std::move(geometry)), source, meta};
}

void MeshLoader::save(ResourceData&) {}

MeshData MeshLoader::load_mesh_data(const SourceMetadata& meta, const ResourceSource& source)
{
    if (meta.format == SourceFormat::Memory)
        return source.load().read<MeshData>();
    if (meta.format == SourceFormat::Obj)
        return load_from_obj(source);

    throw std::runtime_error("Unsupported mesh format");
}

MeshData MeshLoader::load_from_obj(const ResourceSource& source)
{
    const auto data_stream = source.istream();
    std::string content(std::istreambuf_iterator<char>(*data_stream), {});

    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    if (!reader.ParseFromString(content, "", config))
        throw std::runtime_error(std::format("Failed to parse OBJ: {}", reader.Error()));

    if (!reader.Warning().empty())
        logger->warn("OBJ loader warning: {}", reader.Warning());

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    MeshData mesh_data;

    for (const auto& shape : shapes)
    {
        const auto initial_vertex = static_cast<uint32_t>(mesh_data.vertices.size());
        const auto start_index = static_cast<uint32_t>(mesh_data.indices.size());

        struct IndexKey
        {
            int vi, ni, ti;
            bool operator==(const IndexKey&) const = default;
        };

        struct IndexKeyHash
        {
            size_t operator()(const IndexKey& k) const
            {
                size_t h = std::hash<int>{}(k.vi);
                h ^= std::hash<int>{}(k.ni) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= std::hash<int>{}(k.ti) + 0x9e3779b9 + (h << 6) + (h >> 2);
                return h;
            }
        };

        std::unordered_map<IndexKey, uint32_t, IndexKeyHash> unique_vertices;

        for (const auto& idx : shape.mesh.indices)
        {
            IndexKey key{idx.vertex_index, idx.normal_index, idx.texcoord_index};

            if (auto it = unique_vertices.find(key); it != unique_vertices.end())
            {
                mesh_data.indices.push_back(it->second);
                continue;
            }

            Vertex vertex{};
            vertex.position = {
                attrib.vertices[3 * idx.vertex_index + 0],
                attrib.vertices[3 * idx.vertex_index + 1],
                attrib.vertices[3 * idx.vertex_index + 2]
            };

            if (idx.normal_index >= 0)
            {
                vertex.normal = {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };
            }
            else
            {
                vertex.normal = {0.f, 0.f, 1.f};
            }

            if (idx.texcoord_index >= 0)
            {
                vertex.uv_x = attrib.texcoords[2 * idx.texcoord_index + 0];
                vertex.uv_y = attrib.texcoords[2 * idx.texcoord_index + 1];
            }

            auto vertex_index = static_cast<uint32_t>(mesh_data.vertices.size());
            mesh_data.vertices.push_back(vertex);
            unique_vertices[key] = vertex_index;
            mesh_data.indices.push_back(vertex_index);
        }

        MeshGeometryData::Submesh submesh{};
        submesh.start_index = start_index;
        submesh.count = static_cast<uint32_t>(mesh_data.indices.size()) - start_index;

        if (mesh_data.vertices.size() > initial_vertex)
        {
            glm::vec3 min_pos = mesh_data.vertices[initial_vertex].position;
            glm::vec3 max_pos = mesh_data.vertices[initial_vertex].position;
            for (size_t i = initial_vertex; i < mesh_data.vertices.size(); ++i)
            {
                min_pos = glm::min(min_pos, mesh_data.vertices[i].position);
                max_pos = glm::max(max_pos, mesh_data.vertices[i].position);
            }

            submesh.bounds.origin = (max_pos + min_pos) / 2.f;
            submesh.bounds.extents = (max_pos - min_pos) / 2.f;
            submesh.bounds.sphere_radius = glm::length(submesh.bounds.extents);
        }

        mesh_data.submeshes.push_back(submesh);
    }

    calculate_tangents(mesh_data.vertices, mesh_data.indices);

    return mesh_data;
}
} // portal
