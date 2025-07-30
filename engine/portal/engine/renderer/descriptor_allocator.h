//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>


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

    void init(vk::raii::Device* init_device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    void clear_pools();
    void destroy_pools();
    std::vector<vk::raii::DescriptorSet> handle_pool_resize(vk::raii::DescriptorPool& descriptor_pool, vk::DescriptorSetAllocateInfo& info);

    vk::raii::DescriptorSet allocate(const vk::raii::DescriptorSetLayout& layout);

private:
    vk::raii::DescriptorPool get_pool();
    vk::raii::DescriptorPool create_pool(uint32_t set_count, std::span<PoolSizeRatio> pool_ratios) const;

    std::vector<PoolSizeRatio> ratios;
    std::vector<vk::raii::DescriptorPool> full_pools;
    std::vector<vk::raii::DescriptorPool> ready_pools;
    uint32_t sets_per_pool = 0;
    vk::raii::Device* device = nullptr;
};

} // portal
