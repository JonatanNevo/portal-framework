//
// Created by Jonatan Nevo on 02/03/2025.
//

#include "physical_device.h"

namespace portal::vulkan
{
PhysicalDevice::PhysicalDevice(Instance& instance, vk::PhysicalDevice physical_device): instance(instance), handle(physical_device)
{
    features = handle.getFeatures();
    properties = handle.getProperties();
    memory_properties = handle.getMemoryProperties();
    queue_family_properties = handle.getQueueFamilyProperties();
    device_extensions = handle.enumerateDeviceExtensionProperties();

    LOG_CORE_INFO_TAG("Vulkan", "Found GPU: {}", properties.deviceName.data());

    // Display supported extensions
    if (!device_extensions.empty())
    {
        LOG_CORE_DEBUG_TAG("Vulkan", "Device supports the following extensions:");
        for (auto& extension : device_extensions)
        {
            LOG_CORE_DEBUG_TAG("Vulkan", "  \t{}", extension.extensionName.data());
        }
    }
}

DriverVersion PhysicalDevice::get_driver_version() const
{
    DriverVersion version{};

    switch (properties.vendorID)
    {
    case 0x10DE:
        // Nvidia
        version.major = (properties.driverVersion >> 22) & 0x3ff;
        version.minor = (properties.driverVersion >> 14) & 0x0ff;
        version.patch = (properties.driverVersion >> 6) & 0x0ff;
    // Ignoring optional tertiary info in lower 6 bits
        break;
    case 0x8086:
        version.major = (properties.driverVersion >> 14) & 0x3ffff;
        version.minor = properties.driverVersion & 0x3ffff;
        break;
    default:
        version.major = VK_VERSION_MAJOR(properties.driverVersion);
        version.minor = VK_VERSION_MINOR(properties.driverVersion);
        version.patch = VK_VERSION_PATCH(properties.driverVersion);
        break;
    }

    return version;
}

void* PhysicalDevice::get_extension_feature_chain() const { return last_requested_extension_feature; }

bool PhysicalDevice::is_extension_supported(const std::string& requested_extension) const
{
    return std::ranges::find_if(
        device_extensions,
        [requested_extension](auto& device_extension) { return std::strcmp(device_extension.extensionName, requested_extension.c_str()) == 0; }
    ) != device_extensions.end();
}

uint32_t PhysicalDevice::get_queue_family_performance_query_passes(const vk::QueryPoolPerformanceCreateInfoKHR* perf_query_create_info) const
{
    uint32_t passes_needed;
    get_handle().getQueueFamilyPerformanceQueryPassesKHR(perf_query_create_info, &passes_needed);
    return passes_needed;
}

void PhysicalDevice::enumerate_queue_family_performance_query_counters(
    uint32_t queue_family_index,
    uint32_t* count,
    vk::PerformanceCounterKHR* counters,
    vk::PerformanceCounterDescriptionKHR* descriptions
) const
{
    return get_handle().enumerateQueueFamilyPerformanceQueryCountersKHR(queue_family_index);
}

uint32_t PhysicalDevice::get_memory_type(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32* memory_type_found) const
{
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((bits & 1) == 1)
        {
            if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if (memory_type_found)
                    *memory_type_found = true;
                return i;
            }
        }
        bits >>= 1;
    }

    if (memory_type_found)
    {
        *memory_type_found = false;
        return ~0;
    }

    throw std::runtime_error("failed to find suitable memory type");
}
} // portal
