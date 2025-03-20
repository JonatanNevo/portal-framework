//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once

#include "base/builder_base.h"
#include "common.h"
#include "base/allocated.h"

namespace portal::vulkan
{
class Device;
class Buffer;

struct BufferBuilder : public portal::vulkan::allocated::BuilderBase<BufferBuilder, vk::BufferCreateInfo>
{
public:
    explicit BufferBuilder(vk::DeviceSize size);

    Buffer build(Device& device) const;
    std::shared_ptr<Buffer> build_shared(Device& device) const;
    BufferBuilder& with_flags(vk::BufferCreateFlags flags);
    BufferBuilder& with_usage(vk::BufferUsageFlags usage);

private:
    using ParentType = BuilderBase<BufferBuilder, vk::BufferCreateInfo>;

};

class Buffer final : public allocated::Allocated<vk::Buffer>
{
public:
    static Buffer create_staging_buffer(Device& device, vk::DeviceSize size, const void* data);

    template <typename T>
    static Buffer create_staging_buffer(Device& device, const std::vector<T>& data);

    template <typename T>
    static Buffer create_staging_buffer(Device& device, const T& data);

    Buffer() = delete;
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&& other) = default;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = default;

    ~Buffer() override;

    /**
     * @return Return the buffer's device address (note: requires that the buffer has been created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage fla)
     */
    uint64_t get_device_address() const;

    /**
     * @return The size of the buffer
     */
    vk::DeviceSize get_size() const;

protected:
    Buffer(Device& device, const BufferBuilder& builder);
    friend struct BufferBuilder;

private:
    vk::DeviceSize size = 0;
};

template <typename T>
Buffer Buffer::create_staging_buffer(Device& device, const std::vector<T>& data)
{
    return create_staging_buffer(device, data.size() * sizeof(T), data.data());
}

template <typename T>
Buffer Buffer::create_staging_buffer(Device& device, const T& data)
{
    return create_staging_buffer(device, sizeof(T), &data);
}
} // portal
