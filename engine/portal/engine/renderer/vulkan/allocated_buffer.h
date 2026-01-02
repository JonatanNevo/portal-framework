//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/vulkan/base/allocated.h"
#include "portal/engine/renderer/vulkan/base/builder_base.h"

namespace portal::renderer::vulkan
{
class AllocatedBuffer;
class VulkanDevice;

/**
 * @struct BufferBuilder
 * @brief Builder for creating VMA-allocated Vulkan buffers
 *
 * Inherits common VMA options from BuilderBase (vma_usage, vma_flags, etc.).
 * Size is required at construction.
 *
 * Usage:
 * @code
 * auto buffer = BufferBuilder(1024)
 *     .with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
 *     .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
 *     .build(device);
 * @endcode
 */
struct BufferBuilder final : public BuilderBase<BufferBuilder, vk::BufferCreateInfo>
{
public:
    /**
     * @brief Constructs buffer builder with size
     * @param size Buffer size in bytes (required)
     */
    explicit BufferBuilder(vk::DeviceSize size);

    /**
     * @brief Creates AllocatedBuffer
     * @param device The Vulkan device
     * @return AllocatedBuffer with VMA-allocated memory
     */
    AllocatedBuffer build(const VulkanDevice& device) const;

    /**
     * @brief Creates AllocatedBuffer in shared_ptr
     * @param device The Vulkan device
     * @return Shared pointer to AllocatedBuffer
     */
    std::shared_ptr<AllocatedBuffer> build_shared(const VulkanDevice& device) const;

    /**
     * @brief Sets buffer create flags
     * @param flags Buffer create flags
     * @return Reference to this builder
     */
    BufferBuilder& with_flags(vk::BufferCreateFlags flags);

    /**
     * @brief Sets buffer usage flags
     * @param usage Buffer usage flags (eTransferSrc, eStorageBuffer, etc.)
     * @return Reference to this builder
     */
    BufferBuilder& with_usage(vk::BufferUsageFlags usage);

private:
    using ParentType = BuilderBase;
};

/**
 * @class AllocatedBuffer
 * @brief VMA-allocated Vulkan buffer with automatic memory management
 *
 * Inherits from Allocated<vk::Buffer>, providing VMA memory allocation/deallocation,
 * memory mapping, and update methods. Stores buffer size and provides device address query.
 *
 * Destruction automatically calls vmaDestroyBuffer to free both buffer and backing memory.
 */
class AllocatedBuffer final : public allocation::Allocated<vk::Buffer>
{
public:
    /**
     * @brief Creates staging buffer with data
     * @param device The Vulkan device
     * @param size Buffer size in bytes
     * @param data Data to copy into buffer
     * @return AllocatedBuffer with HOST_VISIBLE memory and copied data
     */
    static AllocatedBuffer create_staging_buffer(const VulkanDevice& device, vk::DeviceSize size, const void* data);

    /**
     * @brief Creates staging buffer from std::span
     * @tparam T Element type
     * @param device The Vulkan device
     * @param data Span of data to copy
     * @return AllocatedBuffer with HOST_VISIBLE memory
     */
    template <typename T>
    static AllocatedBuffer create_staging_buffer(const VulkanDevice& device, const std::span<T>& data);

    /**
     * @brief Creates staging buffer from single object
     * @tparam T Object type
     * @param device The Vulkan device
     * @param data Object to copy
     * @return AllocatedBuffer with HOST_VISIBLE memory
     */
    template <typename T>
    static AllocatedBuffer create_staging_buffer(const VulkanDevice& device, const T& data);

    /** @brief Default constructor (creates null buffer) */
    AllocatedBuffer();

    /** @brief Null constructor */
    AllocatedBuffer(std::nullptr_t) : AllocatedBuffer() {}

    /** @brief Move constructor */
    AllocatedBuffer(AllocatedBuffer&& other) noexcept;

    /** @brief Move assignment */
    AllocatedBuffer& operator=(AllocatedBuffer&& other) noexcept;

    /** @brief Null assignment */
    AllocatedBuffer& operator=(std::nullptr_t) noexcept override;


    AllocatedBuffer(const AllocatedBuffer&) = delete;
    AllocatedBuffer& operator=(const AllocatedBuffer&) = delete;


    /** @brief Destructor (calls vmaDestroyBuffer) */
    ~AllocatedBuffer() override;

    /**
     * @brief Gets buffer device address (requires eShaderDeviceAddress usage)
     * @return Buffer device address (for bindless/GPU-driven rendering)
     */
    [[nodiscard]] uint64_t get_device_address() const;

    /**
     * @brief Gets buffer size
     * @return Size in bytes
     */
    [[nodiscard]] vk::DeviceSize get_size() const;

protected:
    /**
     * @brief Protected constructor (use BufferBuilder::build instead)
     * @param device The Vulkan device
     * @param builder Buffer builder with configuration
     */
    AllocatedBuffer(const VulkanDevice& device, const BufferBuilder& builder);
    friend struct BufferBuilder;

private:
    vk::DeviceSize size = 0;
};

template <typename T>
AllocatedBuffer AllocatedBuffer::create_staging_buffer(const VulkanDevice& device, const std::span<T>& data)
{
    return create_staging_buffer(device, data.size() * sizeof(T), data.data());
}

template <typename T>
AllocatedBuffer AllocatedBuffer::create_staging_buffer(const VulkanDevice& device, const T& data)
{
    return create_staging_buffer(device, sizeof(T), &data);
}
} // portal
