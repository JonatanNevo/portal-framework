//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_physical_device.h"

#include "portal/core/log.h"

namespace portal::renderer::vulkan
{

const auto logger = Log::get_logger("Vulkan");

uint32_t rate_device_suitability(const vk::raii::PhysicalDevice& device)
{
    uint32_t score = 0;
    const auto properties = device.getProperties();
    const auto features = device.getFeatures();
    const auto queue_families = device.getQueueFamilyProperties();

    if (std::ranges::find_if(
        queue_families,
        [](const auto& prop)
        {
            return (prop.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
        }
        ) == queue_families.end())
    {
        LOGGER_TRACE("Candidate: {} does not support graphics queue", properties.deviceName.data());
        return 0;
    }

    auto extensions = device.enumerateDeviceExtensionProperties();\
    for (const auto& extension : DEVICE_EXTENSIONS)
    {
        if (std::ranges::find_if(extensions, [extension](auto const& ext) { return strcmp(ext.extensionName, extension) == 0; }) == extensions.end())
        {
            LOGGER_TRACE("Candidate: {} does not support extension {}", extension, extension);
            return 0;
        }
    }

    if (!features.samplerAnisotropy)
    {
        LOGGER_TRACE("Candidate: {} does not support sampler anisotropy", properties.deviceName.data());
        return 0;
    }

    // Discrete GPUs have a significant performance advantage
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
    {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += properties.limits.maxImageDimension2D;

    LOGGER_DEBUG("Available Device: {}", properties.deviceName.data());
    return score;
}

VulkanPhysicalDevice::VulkanPhysicalDevice(const vk::raii::Instance& instance)
{

    const auto physical_devices = instance.enumeratePhysicalDevices();
    if (physical_devices.empty())
    {
        LOGGER_ERROR("No Vulkan physical devices found!");
        // TODO: exit?
    }

    std::multimap<uint32_t, vk::raii::PhysicalDevice> candidates;
    for (const auto& dev : physical_devices)
    {
        uint32_t score = rate_device_suitability(dev);
        LOGGER_DEBUG("Gpu candidate: {} with score {}", dev.getProperties().deviceName.data(), score);
        candidates.insert(std::make_pair(score, dev));
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0)
    {
        physical_device = candidates.rbegin()->second;
        LOGGER_INFO("Picked GPU: {}", physical_device.getProperties().deviceName.data());
    }
    else
    {
        LOGGER_ERROR("Failed to find suitable GPU!");
        // TODO: exit?
    }

    properties = physical_device.getProperties();
    features = physical_device.getFeatures();
    memory_properties = physical_device.getMemoryProperties();
    queue_family_properties = physical_device.getQueueFamilyProperties();

    auto device_extensions = physical_device.enumerateDeviceExtensionProperties();
    LOGGER_TRACE("Physical device has {} extensions: ", device_extensions.size());
    for (auto& [extensionName, specVersion] : device_extensions)
    {
        supported_extensions.emplace(extensionName);
        LOGGER_TRACE("  {} [{}]", extensionName.data(), specVersion);
    }

    // Queue families
    // Desired queues need to be requested upon logical device creation
    // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
    // requests different queue types

    // Get queue family indices for the requested queue family types
    // Note that the indices may overlap depending on the implementation

    static constexpr float default_queue_priority = 0.0f;

    constexpr vk::QueueFlags requested_queue_types = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
    queue_family_indices = get_queue_family_indices(requested_queue_types);


    // Graphics queue
    if (requested_queue_types & vk::QueueFlagBits::eGraphics)
    {
        queue_create_infos.emplace_back(
            vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = static_cast<uint32_t>(queue_family_indices.graphics),
                .queueCount = 1,
                .pQueuePriorities = &default_queue_priority,
            }
            );
    }

    if (requested_queue_types & vk::QueueFlagBits::eCompute)
    {
        if (queue_family_indices.compute != queue_family_indices.graphics)
        {
            queue_create_infos.emplace_back(
                vk::DeviceQueueCreateInfo{
                    .queueFamilyIndex = static_cast<uint32_t>(queue_family_indices.compute),
                    .queueCount = 1,
                    .pQueuePriorities = &default_queue_priority,
                }
                );
        }
    }

    if (requested_queue_types & vk::QueueFlagBits::eTransfer)
    {
        if (queue_family_indices.transfer != queue_family_indices.graphics)
        {
            queue_create_infos.emplace_back(
                vk::DeviceQueueCreateInfo{
                    .queueFamilyIndex = static_cast<uint32_t>(queue_family_indices.transfer),
                    .queueCount = 1,
                    .pQueuePriorities = &default_queue_priority,
                }
                );
        }
    }

    depth_format = find_depth_format();
}

bool VulkanPhysicalDevice::is_extension_supported(const std::string& extensions_name) const
{
    return supported_extensions.contains(extensions_name);
}

vk::raii::PhysicalDevice VulkanPhysicalDevice::get_handle() const
{
    return physical_device;
}

const VulkanPhysicalDevice::QueueFamilyIndices& VulkanPhysicalDevice::get_queue_family_indices() const
{
    return queue_family_indices;
}

const vk::PhysicalDeviceProperties& VulkanPhysicalDevice::get_properties() const
{
    return properties;
}

const vk::PhysicalDeviceFeatures& VulkanPhysicalDevice::get_features() const
{
    return features;
}

const vk::PhysicalDeviceMemoryProperties& VulkanPhysicalDevice::get_memory_properties() const
{
    return memory_properties;
}

vk::Format VulkanPhysicalDevice::get_depth_format() const
{
    return depth_format;
}

vk::Format VulkanPhysicalDevice::find_depth_format() const
{
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector depth_format = {
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD32Sfloat,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD16Unorm
    };

    for (const auto& format : depth_format)
    {
        auto format_prop = physical_device.getFormatProperties(format);
        if (format_prop.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            return format;
    }

    LOGGER_ERROR("Could not find suitable depth format");
    return vk::Format::eUndefined;
}

VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::get_queue_family_indices(vk::QueueFlags queue_flags)
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
