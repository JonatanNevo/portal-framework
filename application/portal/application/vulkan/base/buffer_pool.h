//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once

#include "portal/application/vulkan/buffer.h"

namespace portal::vulkan
{
/**
 * @brief An allocation of vulkan memory; different buffer allocations,
 *        with different offset and size, may come from the same Vulkan buffer
 */
class BufferAllocation
{
public:
    BufferAllocation() = default;
    BufferAllocation(const BufferAllocation&) = delete;
    BufferAllocation(BufferAllocation&&) = default;
    BufferAllocation& operator=(const BufferAllocation&) = delete;
    BufferAllocation& operator=(BufferAllocation&&) = default;

    BufferAllocation(Buffer& buffer, vk::DeviceSize size, vk::DeviceSize offset);

    [[nodiscard]] bool empty() const;
    Buffer& get_buffer() const;
    [[nodiscard]] vk::DeviceSize get_offset() const;
    [[nodiscard]] vk::DeviceSize get_size() const;
    void update(const std::vector<uint8_t>& data, uint32_t offset = 0);
    template <typename T>
    void update(const T& value, uint32_t offset = 0);

private:
    Buffer* buffer = nullptr;
    vk::DeviceSize offset = 0;
    vk::DeviceSize size = 0;
};

template <typename T>
void BufferAllocation::update(const T& value, const uint32_t offset)
{
    update(to_bytes(value), offset);
}


/**
 * @brief Helper class which handles multiple allocation from the same underlying Vulkan buffer.
 */

class BufferBlock
{
public:
    BufferBlock(BufferBlock const& rhs) = delete;
    BufferBlock(BufferBlock&& rhs) = default;
    BufferBlock& operator=(BufferBlock const& rhs) = delete;
    BufferBlock& operator=(BufferBlock&& rhs) = default;

    BufferBlock(Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage);
    /**
     * @return An usable view on a portion of the underlying buffer
     */
    BufferAllocation allocate(vk::DeviceSize size);

    /**
     * @brief check if this BufferBlock can allocate a given amount of memory
     * @param size the number of bytes to check
     * @return \c true if \a size bytes can be allocated from this \c BufferBlock, otherwise \c false.
     */
    [[nodiscard]] bool can_allocate(vk::DeviceSize size) const;

    [[nodiscard]] vk::DeviceSize get_size() const;
    void reset();

private:
    /**
     * @ brief Determine the current aligned offset.
     * @return The current aligned offset.
     */
    [[nodiscard]] vk::DeviceSize aligned_offset() const;
    [[nodiscard]] vk::DeviceSize determine_alignment(vk::BufferUsageFlags usage, vk::PhysicalDeviceLimits const& limits) const;

private:
    Buffer buffer;
    vk::DeviceSize alignment = 0; // Memory alignment, it may change according to the usage
    vk::DeviceSize offset = 0;    // Current offset, it increases on every allocation
};


/**
 * @brief A pool of buffer blocks for a specific usage.
 * It may contain inactive blocks that can be recycled.
 *
 * BufferPool is a linear allocator for buffer chunks, it gives you a view of the size you want.
 * A BufferBlock is the corresponding VkBuffer and you can get smaller offsets inside it.
 * Since a shader cannot specify dynamic UBOs, it has to be done from the code
 * (set_resource_dynamic).
 *
 * When a new frame starts, buffer blocks are returned: the offset is reset and contents are
 * overwritten. The minimum allocation size is 256 kb, if you ask for more you get a dedicated
 * buffer allocation.
 *
 * We re-use descriptor sets: we only need one for the corresponding buffer infos (and we only
 * have one VkBuffer per BufferBlock), then it is bound, and we use dynamic offsets.
 */
class BufferPool
{
public:
    BufferPool(Device& device, vk::DeviceSize block_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage = vma::MemoryUsage::eCpuToGpu);

    BufferBlock& request_buffer_block(vk::DeviceSize minimum_size, bool minimal = false);
    void reset() const;

private:
    Device& device;
    std::vector<std::unique_ptr<BufferBlock>> buffer_blocks;
    /// List of blocks requested (need to be pointers in order to keep their address constant on vector resizing)
    vk::DeviceSize block_size = 0; /// Minimum size of the blocks
    vk::BufferUsageFlags usage;
    vma::MemoryUsage memory_usage{};
};
}
