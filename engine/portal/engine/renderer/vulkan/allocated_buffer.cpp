//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "allocated_buffer.h"

#include "portal/engine/renderer/vulkan/vulkan_device.h"

namespace portal::renderer::vulkan
{
AllocatedBuffer AllocatedBuffer::create_staging_buffer(Ref<VulkanDevice> device, const vk::DeviceSize size, const void* data)
{
    BufferBuilder builder(size);
    builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
           .with_usage(vk::BufferUsageFlagBits::eTransferSrc)
           .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
           .with_debug_name("staging");
    auto buffer = builder.build(device);

    if (data != nullptr)
    {
        [[maybe_unused]] auto copied = buffer.update(data, size);
    }
    return buffer;
}

BufferBuilder::BufferBuilder(const vk::DeviceSize size) : ParentType(vk::BufferCreateInfo{.size = size}) {}

AllocatedBuffer BufferBuilder::build(Ref<VulkanDevice> device) const
{
    return {device, *this};
}

BufferBuilder& BufferBuilder::with_flags(const vk::BufferCreateFlags flags)
{
    this->create_info.flags = flags;
    return *this;
}

BufferBuilder& BufferBuilder::with_usage(const vk::BufferUsageFlags usage)
{
    this->create_info.usage = usage;
    return *this;
}

AllocatedBuffer::AllocatedBuffer() : Allocated({}, nullptr, nullptr) {}

AllocatedBuffer::AllocatedBuffer(AllocatedBuffer&& other) noexcept : Allocated(std::move(other)), size(std::exchange(other.size, 0))
{}

AllocatedBuffer& AllocatedBuffer::operator=(AllocatedBuffer&& other) noexcept
{
    if (this != &other)
    {
        destroy_buffer(get_handle());
        size = std::exchange(other.size, 0);
        Allocated::operator=(std::move(other));
    }
    return *this;
}

AllocatedBuffer& AllocatedBuffer::operator=(nullptr_t) noexcept
{
    destroy_buffer(get_handle());
    Allocated::operator=(nullptr);
    return *this;
}

AllocatedBuffer::~AllocatedBuffer()
{
    destroy_buffer(get_handle());
}

uint64_t AllocatedBuffer::get_device_address() const
{
    const vk::BufferDeviceAddressInfo info{
        .buffer = get_handle(),
    };
    return get_device().get_handle().getBufferAddress(info);
}

vk::DeviceSize AllocatedBuffer::get_size() const
{
    return size;
}

AllocatedBuffer::AllocatedBuffer(Ref<VulkanDevice> device, const BufferBuilder& builder) : Allocated(
                                                                                               builder.get_allocation_create_info(),
                                                                                               nullptr,
                                                                                               device.get()
                                                                                               ),
                                                                                           size(builder.get_create_info().size)
{
    this->set_handle(create_buffer(builder.get_create_info()));

    if (!builder.get_debug_name().empty())
    {
        set_debug_name(builder.get_debug_name());
    }

}

} // portal
