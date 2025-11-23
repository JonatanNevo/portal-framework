//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_uniform_buffer.h"

#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/device/vulkan_physical_device.h"

namespace portal::renderer::vulkan
{
VulkanUniformBuffer::VulkanUniformBuffer(const size_t size, const VulkanDevice& device) : UniformBuffer(INVALID_STRING_ID), size(size), device(device)
{
    init();
}

VulkanUniformBuffer::~VulkanUniformBuffer()
{
    release();
}

void VulkanUniformBuffer::set_data(const Buffer data, const size_t offset)
{
    local_storage.write(data, offset); // TODO: do we need this not in debug?

    [[maybe_unused]] const auto updated = buffer.update(local_storage, 0);
    PORTAL_ASSERT(updated == size, "Failed to update buffer");
}

const Buffer& VulkanUniformBuffer::get_data() const
{
    return local_storage;
}

const vk::DescriptorBufferInfo& VulkanUniformBuffer::get_descriptor_buffer_info() const
{
    return descriptor_buffer_info;
}

void VulkanUniformBuffer::release()
{
    buffer = AllocatedBuffer();
    local_storage = Buffer();
}

void VulkanUniformBuffer::init()
{
    release();

    local_storage = Buffer::allocate(size);
    local_storage.zero_initialize();

    BufferBuilder builder(size);
    builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
           .with_usage(vk::BufferUsageFlagBits::eUniformBuffer)
           .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
           .with_debug_name("uniform_buffer");
    buffer = device.create_buffer(builder);
    [[maybe_unused]] const auto written = buffer.update(local_storage, 0);
    PORTAL_ASSERT(written == size, "Failed to update buffer");

    descriptor_buffer_info = {
        .buffer = buffer.get_handle(),
        .offset = 0,
        .range = size
    };
}

VulkanUniformBufferSet::VulkanUniformBufferSet(size_t buffer_size, const size_t size, const VulkanDevice& device) : UniformBufferSet(INVALID_STRING_ID), device(device)
{
    for (size_t i = 0; i < size; i++)
    {
        buffers[i] = make_reference<VulkanUniformBuffer>(buffer_size, device);
    }
}

Reference<UniformBuffer> VulkanUniformBufferSet::get(const size_t index)
{
    PORTAL_ASSERT(buffers.contains(index), "Invalid buffer index");
    return buffers[index];
}

void VulkanUniformBufferSet::set(const Reference<UniformBuffer>& buffer, const size_t index)
{
    buffers[index] = reference_cast<VulkanUniformBuffer>(buffer);
}

void VulkanUniformBufferSet::set_data([[maybe_unused]] Buffer data, [[maybe_unused]] size_t offset)
{

}

const Buffer& VulkanUniformBufferSet::get_data() const
{
    return buffers.at(0)->get_data();
}

} // portal
