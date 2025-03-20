//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "buffer.h"

#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
BufferBuilder::BufferBuilder(const vk::DeviceSize size): ParentType(vk::BufferCreateInfo({}, size)) {}

Buffer BufferBuilder::build(Device& device) const
{
    return {device, *this};
}

std::shared_ptr<Buffer> BufferBuilder::build_shared(Device& device) const
{
    return std::unique_ptr<Buffer>(new Buffer(device, *this));
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

Buffer Buffer::create_staging_buffer(Device& device, const vk::DeviceSize size, const void* data)
{
    BufferBuilder builder(size);
    builder
        .with_vma_flags(vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite)
        .with_usage(vk::BufferUsageFlagBits::eTransferSrc);
    auto buffer = builder.build(device);

    if (data != nullptr)
        buffer.update(data, size);
    return buffer;
}

Buffer::~Buffer()
{
    destroy_buffer(get_handle());
}

uint64_t Buffer::get_device_address() const
{
    return get_device().get_handle().getBufferAddressKHR(get_handle());
}

vk::DeviceSize Buffer::get_size() const
{
    return size;
}

Buffer::Buffer(Device& device, const BufferBuilder& builder)
    : Allocated(builder.get_allocation_create_info(), nullptr, &device),
      size(builder.get_create_info().size)
{
    this->set_handle(create_buffer(builder.get_create_info()));
    if (!builder.get_debug_name().empty())
        set_debug_name(builder.get_debug_name());
}
} // portal
