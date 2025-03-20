//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "allocated.h"

#include "portal/application/vulkan/device.h"

namespace portal::vulkan::allocated
{
static bool created = false;
static vma::Allocator memory_allocator = nullptr;

vma::Allocator& get_memory_allocator()
{
    return memory_allocator;
}

void init(const Device& device)
{
    vma::VulkanFunctions vma_vulkan_func{};
    vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vma_vulkan_func.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    vma::AllocatorCreateInfo allocator_info{};
    allocator_info.pVulkanFunctions = &vma_vulkan_func;
    allocator_info.physicalDevice = device.get_gpu().get_handle();
    allocator_info.device = device.get_handle();
    allocator_info.instance = device.get_gpu().get_instance().get_handle();

    bool can_get_memory_requirements = device.is_extension_supported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    bool has_dedicated_allocation = device.is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

    if (can_get_memory_requirements && has_dedicated_allocation && device.is_enabled(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
        allocator_info.flags |= vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation;

    if (device.is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) && device.is_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
        allocator_info.flags |= vma::AllocatorCreateFlagBits::eBufferDeviceAddress;

    if (device.is_extension_supported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) && device.is_enabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
        allocator_info.flags |= vma::AllocatorCreateFlagBits::eExtMemoryBudget;

    if (device.is_extension_supported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) && device.is_enabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
        allocator_info.flags |= vma::AllocatorCreateFlagBits::eExtMemoryPriority;

    if (device.is_extension_supported(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) && device.is_enabled(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME))
        allocator_info.flags |= vma::AllocatorCreateFlagBits::eKhrBindMemory2;

    if (device.is_extension_supported(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) && device.
        is_enabled(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME))
        allocator_info.flags |= vma::AllocatorCreateFlagBits::eAmdDeviceCoherentMemory;

    if (!created)
    {
        const auto result = createAllocator(&allocator_info, &memory_allocator);
        if (result != vk::Result::eSuccess)
        {
            memory_allocator = nullptr;
            throw std::runtime_error("Cannot create allocator");
        }
        created = true;
    }
}

void shutdown()
{
    if (created)
    {
        vma::TotalStatistics stats;
        memory_allocator.calculateStatistics(&stats);
        LOG_CORE_INFO_TAG("Vulkan", "Total device memory leak: {} bytes", stats.total.statistics.allocationBytes);
        memory_allocator.destroy();
        memory_allocator = nullptr;
        created = false;
    }
}
}
