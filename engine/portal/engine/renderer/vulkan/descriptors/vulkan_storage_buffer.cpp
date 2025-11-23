//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_storage_buffer.h"

#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/device/vulkan_physical_device.h"

namespace portal::renderer::vulkan
{

VulkanStorageBuffer::VulkanStorageBuffer(const StorageBufferProperties& properties, const VulkanDevice& device)
    : StorageBuffer(properties.debug_name),
      device(device),
      properties(properties)
{
    init();
}

VulkanStorageBuffer::~VulkanStorageBuffer()
{
    release();
}

void VulkanStorageBuffer::set_data(const Buffer data, const size_t offset)
{
    PORTAL_ASSERT(!properties.gpu_only, "Cannot set data on a GPU only buffer");

    local_storage.write(data, offset);

    [[maybe_unused]] const auto updated = buffer.update(local_storage, 0);
    PORTAL_ASSERT(updated == properties.size, "Failed to update buffer");
}

const Buffer& VulkanStorageBuffer::get_data() const
{
    return local_storage;
}

void VulkanStorageBuffer::resize(const size_t new_size)
{
    properties.size = new_size;
    init();
}

vk::DescriptorBufferInfo& VulkanStorageBuffer::get_descriptor_buffer_info()
{
    return descriptor_buffer_info;
}

void VulkanStorageBuffer::release()
{
    buffer = AllocatedBuffer();
    local_storage = Buffer();
}

void VulkanStorageBuffer::init()
{
    release();

    BufferBuilder builder(properties.size);
    builder.with_usage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer)
           .with_debug_name(properties.debug_name.string.data());

    if (properties.gpu_only)
    {
        builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);
    }
    else
    {
        builder.with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
               .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT);
    }

    buffer = device.create_buffer(builder);

    local_storage = Buffer::allocate(properties.size);
    descriptor_buffer_info = {
        .buffer = buffer.get_handle(),
        .offset = 0,
        .range = properties.size
    };
}


VulkanStorageBufferSet::VulkanStorageBufferSet(const size_t buffer_size, const size_t size, const VulkanDevice& device) : StorageBufferSet(
    INVALID_STRING_ID
    )
{
    for (size_t i = 0; i < size; i++)
    {
        buffers[i] = make_reference<VulkanStorageBuffer>(
            StorageBufferProperties{buffer_size, true, STRING_ID(std::format("sub_storage_{}", i))},
            device
            );
    }
}


Reference<StorageBuffer> VulkanStorageBufferSet::get(const size_t index)
{
    PORTAL_ASSERT(buffers.contains(index), "Invalid buffer index");
    return buffers[index];
}

void VulkanStorageBufferSet::set(const Reference<StorageBuffer>& buffer, const size_t index)
{
    buffers[index] = reference_cast<VulkanStorageBuffer>(buffer);
}

void VulkanStorageBufferSet::set_data([[maybe_unused]] Buffer data, [[maybe_unused]] size_t offset) {
}

const Buffer& VulkanStorageBufferSet::get_data() const
{
    return buffers.at(0)->get_data();
}
} // portal
