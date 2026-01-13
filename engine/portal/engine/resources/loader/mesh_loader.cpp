//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "mesh_loader.h"

#include "portal/engine/resources/source/resource_source.h"

namespace portal::resources
{
MeshLoader::MeshLoader(ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context) : ResourceLoader(registry), context(context)
{}

Reference<Resource> MeshLoader::load(const SourceMetadata& meta, const ResourceSource& source)
{
    auto [vertices, indexes, submeshes] = load_mesh_data(meta, source);

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

    return make_reference<MeshGeometry>(meta.resource_id, std::move(geometry));
}

MeshData MeshLoader::load_mesh_data(const SourceMetadata& meta, const ResourceSource& source)
{
    if (meta.format == SourceFormat::Memory)
        return source.load().read<MeshData>();

    throw std::runtime_error("Unsupported mesh format");
}
} // portal
