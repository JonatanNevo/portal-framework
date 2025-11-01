//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <memory>


namespace portal::vulkan
{

class DescriptorAllocator
{
public:
    struct PoolSizeRatio
    {
        vk::DescriptorType type;
        float ratio;
    };

    DescriptorAllocator(const vk::raii::Device& device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);

    ~DescriptorAllocator() = default;
    DescriptorAllocator(const DescriptorAllocator&) = delete;
    DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

    DescriptorAllocator(DescriptorAllocator&& other) noexcept;
    DescriptorAllocator& operator=(DescriptorAllocator&& other) noexcept;

    void clear_pools();
    void destroy_pools();
    std::vector<vk::raii::DescriptorSet> handle_pool_resize(vk::raii::DescriptorPool& descriptor_pool, vk::DescriptorSetAllocateInfo& info);

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
