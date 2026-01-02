//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <memory>


namespace portal::renderer::vulkan
{
/**
 * @class DescriptorAllocator
 * @brief Pooled descriptor set allocator with automatic resizing
 *
 * Manages descriptor pools, automatically creating new pools when capacity is exhausted.
 * Uses two-pool system (ready_pools/full_pools) and resets all pools each frame via clear_pools().
 * When allocation fails, creates new pool at 1.5x size.
 */
class DescriptorAllocator
{
public:
    /** @brief Descriptor type to count ratio for pool sizing */
    struct PoolSizeRatio
    {
        vk::DescriptorType type;
        float ratio;
    };

    /**
     * @brief Constructs descriptor allocator
     * @param device Vulkan device
     * @param max_sets Initial maximum sets per pool
     * @param pool_ratios Descriptor type ratios
     */
    DescriptorAllocator(const vk::raii::Device& device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);

    ~DescriptorAllocator() = default;
    DescriptorAllocator(const DescriptorAllocator&) = delete;
    DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

    DescriptorAllocator(DescriptorAllocator&& other) noexcept;
    DescriptorAllocator& operator=(DescriptorAllocator&& other) noexcept;

    /**
     * @brief Resets all pools (call each frame)
     */
    void clear_pools();

    /**
     * @brief Destroys all pools
     */
    void destroy_pools();

    /**
     * @brief Handles pool resize when allocation fails
     * @param descriptor_pool The full pool
     * @param info Allocation info
     * @return Descriptor sets from new pool
     */
    std::vector<vk::raii::DescriptorSet> handle_pool_resize(vk::raii::DescriptorPool& descriptor_pool, vk::DescriptorSetAllocateInfo& info);

    /**
     * @brief Allocates descriptor set (auto-creates new pool if needed)
     * @param layout Descriptor set layout
     * @return Allocated descriptor set
     */
    vk::raii::DescriptorSet allocate(const vk::DescriptorSetLayout& layout);

private:
    vk::raii::DescriptorPool get_pool();
    [[nodiscard]] vk::raii::DescriptorPool create_pool(uint32_t set_count, std::span<PoolSizeRatio> pool_ratios) const;

private:
    std::vector<PoolSizeRatio> ratios;
    std::vector<vk::raii::DescriptorPool> full_pools;
    std::vector<vk::raii::DescriptorPool> ready_pools;
    uint32_t sets_per_pool = 0;
    const vk::raii::Device& device;
};
} // portal
