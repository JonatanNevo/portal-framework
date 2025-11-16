//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <unordered_set>
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/device/physical_device.h"


namespace portal::renderer::vulkan
{

class VulkanDevice;

class VulkanPhysicalDevice final : public PhysicalDevice
{
public:
    struct QueueFamilyIndices
    {
        int32_t graphics = -1;
        int32_t compute = -1;
        int32_t transfer = -1;
    };

    using Features = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >;

public:
    explicit VulkanPhysicalDevice(vk::raii::PhysicalDevice&& physical_device);

    [[nodiscard]] vk::Format find_depth_format() const;

    [[nodiscard]] DriverVersion get_driver_version() const override;
    [[nodiscard]] bool is_extension_supported(std::string_view extensions_name) const override;
    [[nodiscard]] bool supports_present(Surface& surface, uint32_t queue_family_index) const override;

    [[nodiscard]] const Features& get_features_chain() const { return features_chain; }
    [[nodiscard]] vk::FormatProperties get_format_properties(vk::Format format) const;
    [[nodiscard]] const vk::raii::PhysicalDevice& get_handle() const { return handle; }
    [[nodiscard]] vk::PhysicalDeviceFeatures get_features() const;
    [[nodiscard]] vk::PhysicalDeviceProperties get_properties() const { return properties; }
    [[nodiscard]] vk::PhysicalDeviceMemoryProperties get_memory_properties() const { return memory_properties; }

    [[nodiscard]] const std::vector<vk::QueueFamilyProperties>& get_queue_family_properties() const;

    [[nodiscard]] QueueFamilyIndices get_queue_family_indices(vk::QueueFlags queue_flags) const;

private:
    vk::raii::PhysicalDevice handle = nullptr;

    Features features_chain;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceMemoryProperties memory_properties;
    std::vector<vk::QueueFamilyProperties> queue_family_properties;

    std::unordered_set<std::string> supported_extensions;
};

}
