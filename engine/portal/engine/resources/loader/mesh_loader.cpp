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

            vertex.color = {1.f, 1.f, 1.f, 1.f};

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

    return mesh_data;
}
} // portal
