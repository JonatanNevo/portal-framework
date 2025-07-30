//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "base/allocated.h"
#include "base/builder_base.h"

namespace portal::vulkan
{
class AllocatedBuffer;

struct BufferBuilder final : public BuilderBase<BufferBuilder, vk::BufferCreateInfo>
{
public:
    explicit BufferBuilder(vk::DeviceSize size);

    AllocatedBuffer build(vk::raii::Device& device) const;
    std::shared_ptr<AllocatedBuffer> build_shared(vk::raii::Device& device) const;
    BufferBuilder& with_flags(vk::BufferCreateFlags flags);
    BufferBuilder& with_usage(vk::BufferUsageFlags usage);

private:
    using ParentType = BuilderBase;
};

class AllocatedBuffer final : public allocation::Allocated<vk::Buffer>
{
public:
    static AllocatedBuffer create_staging_buffer(vk::raii::Device& device, vk::DeviceSize size, const void* data);

    template <typename T>
    static AllocatedBuffer create_staging_buffer(vk::raii::Device& device, const std::span<T>& data);

    template <typename T>
    static AllocatedBuffer create_staging_buffer(vk::raii::Device& device, const T& data);

    AllocatedBuffer();
    AllocatedBuffer(nullptr_t): AllocatedBuffer() {}

    AllocatedBuffer(AllocatedBuffer&& other) noexcept;
    AllocatedBuffer& operator=(AllocatedBuffer&& other) noexcept;
    AllocatedBuffer& operator=(nullptr_t) noexcept override;


    AllocatedBuffer(const AllocatedBuffer&) = delete;
    AllocatedBuffer& operator=(const AllocatedBuffer&) = delete;


    ~AllocatedBuffer() override;

    /**
     * @return Return the buffer's device address
     */
    [[nodiscard]] uint64_t get_device_address() const;

    /**
     * @return The size of the buffer
     */
    [[nodiscard]] vk::DeviceSize get_size() const;

protected:
    AllocatedBuffer(vk::raii::Device& device, const BufferBuilder& builder);
    friend struct BufferBuilder;

private:
    vk::DeviceSize size = 0;
};

template <typename T>
AllocatedBuffer AllocatedBuffer::create_staging_buffer(vk::raii::Device& device, const std::span<T>& data)
{
    return create_staging_buffer(device, data.size() * sizeof(T), data.data());
}

template <typename T>
AllocatedBuffer AllocatedBuffer::create_staging_buffer(vk::raii::Device& device, const T& data)
{
    return create_staging_buffer(device, sizeof(T), &data);
}
} // portal
