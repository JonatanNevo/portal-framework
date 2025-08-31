//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <unordered_set>
#include <vulkan/vulkan_raii.hpp>

#include "portal/core/reference.h"

namespace portal::renderer::vulkan
{

constexpr std::array DEVICE_EXTENSIONS = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
    vk::EXTCalibratedTimestampsExtensionName,
#if defined(PORTAL_PLATFORM_MACOS)
    vk::KHRPortabilitySubsetExtensionName
#endif
};

class VulkanDevice;

class VulkanPhysicalDevice final : public RefCounted
{
public:
    struct QueueFamilyIndices
    {
        int32_t graphics = -1;
        int32_t compute = -1;
        int32_t transfer = -1;
    };

public:
    explicit VulkanPhysicalDevice(const vk::raii::Instance& instance);

    bool is_extension_supported(const std::string& extensions_name) const;

    vk::raii::PhysicalDevice get_handle() const;
    const QueueFamilyIndices& get_queue_family_indices() const;

    const vk::PhysicalDeviceProperties& get_properties() const;
    const vk::PhysicalDeviceFeatures& get_features() const;
    const vk::PhysicalDeviceMemoryProperties& get_memory_properties() const;

    vk::Format get_depth_format() const;


private:
    vk::Format find_depth_format() const;
    QueueFamilyIndices get_queue_family_indices(vk::QueueFlags queue_flags);

private:
    friend class VulkanDevice;

    QueueFamilyIndices queue_family_indices;

    vk::raii::PhysicalDevice physical_device = nullptr;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceFeatures features;
    vk::PhysicalDeviceMemoryProperties memory_properties;

    vk::Format depth_format;

    std::vector<vk::QueueFamilyProperties> queue_family_properties;
    std::unordered_set<std::string> supported_extensions;
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

};

}
