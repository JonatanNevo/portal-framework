//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "buffer_pool.h"

#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
BufferAllocation::BufferAllocation(Buffer& buffer, const vk::DeviceSize size, const vk::DeviceSize offset)
    : buffer(&buffer),
      offset(offset),
      size(size)
{}

bool BufferAllocation::empty() const
{
    return size == 0 || buffer == nullptr;
}

Buffer& BufferAllocation::get_buffer() const
{
    return *buffer;
}

vk::DeviceSize BufferAllocation::get_offset() const
{
    return offset;
}

vk::DeviceSize BufferAllocation::get_size() const
{
    return size;
}

void BufferAllocation::update(const std::vector<uint8_t>& data, uint32_t offset)
{
    if (offset + data.size() <= size)
    {
        buffer->update(data, this->offset + offset);
    }
    else
    {
        LOG_CORE_ERROR_TAG("Vulkan", "Ignore buffer allocation update");
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BufferBlock::BufferBlock(Device& device, const vk::DeviceSize size, const vk::BufferUsageFlags usage, const vma::MemoryUsage memory_usage)
    : buffer(BufferBuilder(size).with_usage(usage).with_vma_usage(memory_usage).build(device))
{
    alignment = determine_alignment(usage, device.get_gpu().get_properties().limits);
}

BufferAllocation BufferBlock::allocate(vk::DeviceSize size)
{
    if (can_allocate(size))
    {
        // Move the current offset and return an allocation
        const auto aligned = aligned_offset();
        offset = aligned + size;
        return {buffer, size, aligned};
    }

    // No more space available from the underlying buffer, return empty allocation
    return BufferAllocation{};
}

bool BufferBlock::can_allocate(const vk::DeviceSize size) const
{
    PORTAL_CORE_ASSERT(size > 0, "Allocation size must be greater than 0");
    return aligned_offset() + size <= buffer.get_size();
}

vk::DeviceSize BufferBlock::get_size() const
{
    return buffer.get_size();
}

void BufferBlock::reset()
{
    offset = 0;
}

vk::DeviceSize BufferBlock::aligned_offset() const
{
    return (offset + alignment - 1) & ~(alignment - 1);
}

vk::DeviceSize BufferBlock::determine_alignment(vk::BufferUsageFlags usage, vk::PhysicalDeviceLimits const& limits) const
{
    if (usage == vk::BufferUsageFlagBits::eUniformBuffer)
        return limits.minUniformBufferOffsetAlignment;

    if (usage == vk::BufferUsageFlagBits::eStorageBuffer)
        return limits.minStorageBufferOffsetAlignment;

    if (usage == vk::BufferUsageFlagBits::eUniformTexelBuffer)
        return limits.minTexelBufferOffsetAlignment;
    if (usage == vk::BufferUsageFlagBits::eIndexBuffer || usage == vk::BufferUsageFlagBits::eVertexBuffer || usage ==
        vk::BufferUsageFlagBits::eIndirectBuffer)
    {
        // Used to calculate the offset, required when allocating memory (its value should be power of 2)
        return 16;
    }

    throw std::runtime_error("Usage not recognised");
}

BufferPool::BufferPool(Device& device, const vk::DeviceSize block_size, const vk::BufferUsageFlags usage, const vma::MemoryUsage memory_usage)
    : device(device), block_size(block_size), usage(usage), memory_usage(memory_usage)
{}

BufferBlock& BufferPool::request_buffer_block(vk::DeviceSize minimum_size, bool minimal)
{
    // Find a block in the range of the blocks which can fit the minimum size
    auto it = minimal
                  ? std::ranges::find_if(
                      buffer_blocks,
                      [&minimum_size](auto const& buffer_block)
                      {
                          return (buffer_block->get_size() == minimum_size) && buffer_block->can_allocate(minimum_size);
                      }
                  )
                  : std::ranges::find_if(
                      buffer_blocks,
                      [&minimum_size](auto const& buffer_block) { return buffer_block->can_allocate(minimum_size); }
                  );

    if (it == buffer_blocks.end())
    {
        LOG_CORE_DEBUG_TAG("Vulkan", "Building #{} buffer block ({})", buffer_blocks.size(), vk::to_string(usage));

        vk::DeviceSize new_block_size = minimal ? minimum_size : (std::max)(block_size, minimum_size);

        // Create a new block and get the iterator on it
        it = buffer_blocks.emplace(buffer_blocks.end(), std::make_unique<BufferBlock>(device, new_block_size, usage, memory_usage));
    }

    return *it->get();
}

void BufferPool::reset() const
{
    // Attention: Resetting the BufferPool is not supposed to clear the BufferBlocks, but just reset them!
    //						The actual VkBuffers are used to hash the DescriptorSet in RenderFrame::request_descriptor_set.
    //						Don't know (for now) how that works with resetted buffers!
    for (const auto& buffer_block : buffer_blocks)
    {
        buffer_block->reset();
    }
}
}
