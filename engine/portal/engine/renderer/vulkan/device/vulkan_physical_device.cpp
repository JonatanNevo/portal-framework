//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_physical_device.h"
#include <ranges>

#include "portal/core/log.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/surface/vulkan_surface.h"

namespace portal::renderer::vulkan
{
const auto logger = Log::get_logger("Vulkan");

VulkanPhysicalDevice::VulkanPhysicalDevice(vk::raii::PhysicalDevice&& physical_device) : handle(
    physical_device
)
{
    const auto available_features = physical_device.getFeatures();
    features_chain = {
        {
            .features = {
                .independentBlend = available_features.independentBlend,
                .sampleRateShading = available_features.sampleRateShading,
                .fillModeNonSolid = available_features.fillModeNonSolid,
                .wideLines = available_features.wideLines,
                .samplerAnisotropy = available_features.samplerAnisotropy,
                .pipelineStatisticsQuery = available_features.pipelineStatisticsQuery,
                .shaderStorageImageReadWithoutFormat = available_features.shaderStorageImageReadWithoutFormat,
            }
        },
        {
            .shaderDrawParameters = true
        },
        {
            .bufferDeviceAddress = true
        },
        {
            .synchronization2 = true,
            .dynamicRendering = true
        },
        {
            .extendedDynamicState = true
        }
    };

    properties = physical_device.getProperties();
    memory_properties = physical_device.getMemoryProperties();
    queue_family_properties = physical_device.getQueueFamilyProperties();

    LOGGER_INFO("Initializing physical device: {}", properties.deviceName.data());
    auto device_extensions = physical_device.enumerateDeviceExtensionProperties();
    LOGGER_TRACE("Physical device has {} extensions: ", device_extensions.size());
    for (auto& [extension_name, spec_version] : device_extensions)
    {
        supported_extensions.emplace(std::string{extension_name.data(), extension_name.size()});
        LOGGER_TRACE("  {} [v{}]", extension_name.data(), spec_version);
    }

    // depth_format = find_depth_format();
}

vk::Format VulkanPhysicalDevice::find_depth_format() const
{
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector possible_depth_formats = {
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD32Sfloat,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD16Unorm
    };

    for (const auto& format : possible_depth_formats)
    {
        auto format_prop = handle.getFormatProperties(format);
        if (format_prop.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            return format;
    }

    LOGGER_ERROR("Could not find suitable depth format");
    return vk::Format::eUndefined;
}

DriverVersion VulkanPhysicalDevice::get_driver_version() const
{
    DriverVersion version{};

    switch (properties.vendorID)
    {
    case 0x10DE:
        // Nvidia
        version.major = static_cast<uint16_t>(properties.driverVersion >> 22) & 0x3ff;
        version.minor = static_cast<uint16_t>(properties.driverVersion >> 14) & 0x0ff;
        version.patch = static_cast<uint16_t>(properties.driverVersion >> 6) & 0x0ff;
        // Ignoring optional tertiary info in lower 6 bits
        break;
    case 0x8086:
        version.major = static_cast<uint16_t>(properties.driverVersion >> 14) & 0x3ffff;
        version.minor = static_cast<uint16_t>(properties.driverVersion) & 0x3ffff;
        break;
    default:
        version.major = static_cast<uint16_t>(VK_VERSION_MAJOR(properties.driverVersion));
        version.minor = static_cast<uint16_t>(VK_VERSION_MINOR(properties.driverVersion));
        version.patch = static_cast<uint16_t>(VK_VERSION_PATCH(properties.driverVersion));
        break;
    }

    return version;
}

bool VulkanPhysicalDevice::is_extension_supported(std::string_view extensions_name) const
{
    return std::ranges::find_if(
        supported_extensions,
        [extensions_name](const auto& ext) { return std::strcmp(ext.data(), extensions_name.data()) == 0; }
    ) != supported_extensions.end();
}

bool VulkanPhysicalDevice::supports_present(Surface& surface, const uint32_t queue_family_index) const
{
    const auto& vulkan_surface = dynamic_cast<VulkanSurface&>(surface);
    return handle.getSurfaceSupportKHR(queue_family_index, vulkan_surface.get_vulkan_surface());
}

vk::PhysicalDeviceFeatures VulkanPhysicalDevice::get_features() const
{
    return handle.getFeatures();
}

vk::FormatProperties VulkanPhysicalDevice::get_format_properties(const vk::Format format) const
{
    return handle.getFormatProperties(format);
}

const std::vector<vk::QueueFamilyProperties>& VulkanPhysicalDevice::get_queue_family_properties() const
{
    return queue_family_properties;
}

VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::get_queue_family_indices(vk::QueueFlags queue_flags) const
{
    QueueFamilyIndices indices;

    auto find_dedicated_family_type = [&](vk::QueueFlags queue_type)
    {
        const auto it = std::ranges::find_if(
            queue_family_properties,
            [queue_type](const auto& prop)
            {
                return (
                    (prop.queueFlags & queue_type) != static_cast<vk::QueueFlags>(0) &&
                    (prop.queueFlags & vk::QueueFlagBits::eGraphics) == static_cast<vk::QueueFlags>(0)
                );
            }
        );

        if (it == queue_family_properties.end())
            return -1;

        return static_cast<int32_t>(std::distance(queue_family_properties.begin(), it));
    };

    // Dedicated queue for compute
    // Try to find a queue family index that supports compute but not graphics
    if (queue_flags & vk::QueueFlagBits::eCompute)
    {
        indices.compute = find_dedicated_family_type(vk::QueueFlagBits::eCompute);
    }

    // Dedicated queue for transfer
    // Try to find a queue family index that supports transfer but not graphics and compute
    if (queue_flags & vk::QueueFlagBits::eTransfer)
    {
        indices.compute = find_dedicated_family_type(vk::QueueFlagBits::eCompute);
    }

    for (uint32_t i = 0; i < queue_family_properties.size(); i++)
    {
        if (queue_flags & vk::QueueFlagBits::eTransfer && indices.transfer == -1)
        {
            if (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eTransfer)
                indices.transfer = i;
        }

        if (queue_flags & vk::QueueFlagBits::eCompute && indices.compute == -1)
        {
            if (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eCompute)
                indices.compute = i;
        }


        if (queue_flags & vk::QueueFlagBits::eGraphics)
        {
            if (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
                indices.graphics = i;
        }
    }

    return indices;
}
} // portal
