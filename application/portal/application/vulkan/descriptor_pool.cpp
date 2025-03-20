//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "descriptor_pool.h"
#include <vulkan/vulkan_hash.hpp>

#include "portal/application/vulkan/descriptor_set_layout.h"
#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
DescriptorPool::DescriptorPool(Device& device, const DescriptorSetLayout& descriptor_set_layout, uint32_t pool_size)
    : device(device),
      descriptor_set_layout(&descriptor_set_layout)
{
    const auto& bindings = descriptor_set_layout.get_bindings();
    std::map<vk::DescriptorType, std::uint32_t> descriptor_type_counts;

    // Count each type of descriptor set
    for (auto& binding : bindings)
    {
        descriptor_type_counts[binding.descriptorType] += binding.descriptorCount;
    }

    // Allocate pool sizes array
    pool_sizes.resize(descriptor_type_counts.size());

    auto pool_size_it = pool_sizes.begin();
    // Fill pool size for each descriptor type count multiplied by the pool size
    for (auto& [type, count] : descriptor_type_counts)
    {
        pool_size_it->type = type;
        pool_size_it->descriptorCount = count * pool_size;

        ++pool_size_it;
    }

    pool_max_sets = pool_size;
}

DescriptorPool::~DescriptorPool()
{
    // Destroy all descriptor pools
    for (const auto pool : pools)
    {
        device.get_handle().destroyDescriptorPool(pool);
    }
}

void DescriptorPool::reset()
{
    // Reset all descriptor pools
    for (const auto pool : pools)
    {
        device.get_handle().resetDescriptorPool(pool);
    }
    std::ranges::fill(pool_sets_count, 0);
    set_pool_mapping.clear();

    // Reset the pool index from which descriptor sets are allocated
    pool_index = 0;
}

const DescriptorSetLayout& DescriptorPool::get_descriptor_set_layout() const
{
    return *descriptor_set_layout;
}

void DescriptorPool::set_descriptor_set_layout(const DescriptorSetLayout& set_layout)
{
    descriptor_set_layout = &set_layout;
}

vk::DescriptorSet DescriptorPool::allocate()
{
    pool_index = find_available_pool(pool_index);

    // Increment allocated set count for the current pool
    ++pool_sets_count[pool_index];

    const vk::DescriptorSetLayout layout = descriptor_set_layout->get_handle();

    const vk::DescriptorSetAllocateInfo info(pools[pool_index], 1, &layout);
    const auto handle = device.get_handle().allocateDescriptorSets(info).front();
    if (!handle)
    {
        // Decrement allocated set count for the current pool
        --pool_sets_count[pool_index];
        return VK_NULL_HANDLE;
    }

    // Store mapping between the descriptor set and the pool
    set_pool_mapping.emplace(handle, pool_index);
    return handle;
}

vk::Result DescriptorPool::free(const vk::DescriptorSet descriptor_set)
{
    // Get the pool index of the descriptor set
    auto it = set_pool_mapping.find(descriptor_set);
    if (it == set_pool_mapping.end())
        return vk::Result::eIncomplete;

    const auto desc_pool_index = it->second;
    // Free descriptor set from the pool
    device.get_handle().freeDescriptorSets(pools[desc_pool_index], 1, &descriptor_set);
    // Remove descriptor set mapping to the pool
    set_pool_mapping.erase(it);
    // Decrement allocated set count for the pool
    --pool_sets_count[desc_pool_index];
    // Change the current pool index to use the available pool
    pool_index = desc_pool_index;

    return vk::Result::eSuccess;
}

uint32_t DescriptorPool::find_available_pool(uint32_t search_index)
{
    // Create a new pool
    if (pools.size() <= search_index)
    {
        vk::DescriptorPoolCreateInfo pool_info({}, pool_max_sets, pool_sizes);

        // Check descriptor set layout and enable the required flags
        auto& binding_flags = descriptor_set_layout->get_binding_flags();
        for (auto binding_flag : binding_flags)
        {
            if (binding_flag & vk::DescriptorBindingFlagBits::eUpdateAfterBind)
                pool_info.flags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
        }

        const auto pool = device.get_handle().createDescriptorPool(pool_info);
        if (!pool)
            return 0;

        // Store internally the Vulkan handle
        pools.push_back(pool);
        // Add set count for the descriptor pool
        pool_sets_count.push_back(0);
        return search_index;
    }
    if (pool_sets_count[search_index] < pool_max_sets)
        return search_index;

    // Increment pool index
    return find_available_pool(++search_index);
}
} // portal
