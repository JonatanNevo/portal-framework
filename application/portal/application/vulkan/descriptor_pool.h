//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once

#include <unordered_map>

#include "portal/application/vulkan/common.h"

namespace portal::vulkan
{
class Device;
class DescriptorSetLayout;

class DescriptorPool
{
public:
    static constexpr uint32_t MAX_SETS_PER_POOL = 16;

    DescriptorPool(Device& device, const DescriptorSetLayout& descriptor_set_layout, uint32_t pool_size = MAX_SETS_PER_POOL);
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&&) = default;
    DescriptorPool& operator=(const DescriptorPool&) = delete;
    DescriptorPool& operator=(DescriptorPool&&) = delete;

    void reset();
    const DescriptorSetLayout &get_descriptor_set_layout() const;
    void set_descriptor_set_layout(const DescriptorSetLayout &set_layout);
    vk::DescriptorSet allocate();
    vk::Result free(vk::DescriptorSet descriptor_set);

private:
    // Find next pool index or create new pool
    uint32_t find_available_pool(uint32_t search_index);

    Device &device;
    const DescriptorSetLayout* descriptor_set_layout = nullptr;
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    // Number of sets to allocate for each pool
    uint32_t pool_max_sets = 0;
    // Total descriptor pools created
    std::vector<vk::DescriptorPool> pools;
    // Count sets for each pool
    std::vector<uint32_t> pool_sets_count;
    // Current pool index to allocate descriptor set
    uint32_t pool_index = 0;
    // Map between descriptor set and pool index
    std::map<vk::DescriptorSet, uint32_t> set_pool_mapping;
};
} // portal
