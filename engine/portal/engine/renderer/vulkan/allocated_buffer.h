//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/reference.h"
#include "portal/engine/renderer/vulkan/base/allocated.h"
#include "portal/engine/renderer/vulkan/base/builder_base.h"

namespace portal::renderer::vulkan
{
class AllocatedBuffer;
class VulkanDevice;

struct BufferBuilder final : public BuilderBase<BufferBuilder, vk::BufferCreateInfo>
{
public:
    explicit BufferBuilder(vk::DeviceSize size);

    AllocatedBuffer build(const Ref<VulkanDevice>& device) const;
    std::shared_ptr<AllocatedBuffer> build_shared(const Ref<VulkanDevice>& device) const;
    BufferBuilder& with_flags(vk::BufferCreateFlags flags);
    BufferBuilder& with_usage(vk::BufferUsageFlags usage);

private:
    using ParentType = BuilderBase;
};

class AllocatedBuffer final : public allocation::Allocated<vk::Buffer>
{
public:
    static AllocatedBuffer create_staging_buffer(Ref<VulkanDevice> device, vk::DeviceSize size, const void* data);

    template <typename T>
    static AllocatedBuffer create_staging_buffer(Ref<VulkanDevice> device, const std::span<T>& data);

    template <typename T>
    static AllocatedBuffer create_staging_buffer(Ref<VulkanDevice> device, const T& data);

    AllocatedBuffer();
    AllocatedBuffer(nullptr_t) : AllocatedBuffer() {}

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
    AllocatedBuffer(Ref<VulkanDevice> device, const BufferBuilder& builder);
    friend struct BufferBuilder;

private:
    vk::DeviceSize size = 0;
};

template <typename T>
AllocatedBuffer AllocatedBuffer::create_staging_buffer(Ref<VulkanDevice> device, const std::span<T>& data)
{
    return create_staging_buffer(device, data.size() * sizeof(T), data.data());
}

template <typename T>
AllocatedBuffer AllocatedBuffer::create_staging_buffer(Ref<VulkanDevice> device, const T& data)
{
    return create_staging_buffer(device, sizeof(T), &data);
}
} // portal
