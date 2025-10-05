//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "allocated.h"

#include "portal/engine/renderer/vulkan/vulkan_common.h"

namespace portal::renderer::vulkan::allocation
{
static bool created = false;
static VmaAllocator memory_allocator = nullptr;

VmaAllocator& get_vma_allocator()
{
    return memory_allocator;
}

void init(const vk::Instance& instance, const vk::PhysicalDevice& physical_device, const vk::Device& device)
{
    VmaVulkanFunctions vma_vulkan_functions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo allocator_create_info{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physical_device,
        .device = device,
        .pVulkanFunctions = &vma_vulkan_functions,
        .instance = instance,
    };

    if (!created)
    {
        const auto result = vmaCreateAllocator(&allocator_create_info, &memory_allocator);
        if (result != VK_SUCCESS)
        {
            memory_allocator = nullptr;
            throw std::runtime_error("Failed to create VMA allocator");
        }
        created = true;
    }
}

void shutdown()
{
    if (created)
    {
        VmaTotalStatistics stats;
        vmaCalculateStatistics(memory_allocator, &stats);
        LOG_INFO_TAG("Vulkan", "Total device memory leak: {} bytes", stats.total.statistics.allocationBytes);
        vmaDestroyAllocator(memory_allocator);
        memory_allocator = nullptr;
        created = false;
    }
}
} // portal::vulkan::allocation
