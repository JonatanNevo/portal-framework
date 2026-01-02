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

/**
 * @class VulkanPhysicalDevice
 * @brief Vulkan physical device (GPU) wrapper with capabilities and feature queries
 *
 * Wraps vk::raii::PhysicalDevice and caches properties, features, memory properties,
 * queue families, and supported extensions at construction time. Provides queries for
 * GPU capabilities needed during device creation and resource allocation.
 */
class VulkanPhysicalDevice final : public PhysicalDevice
{
public:
    /** @brief Queue family indices for different queue types */
    struct QueueFamilyIndices
    {
        int32_t graphics = -1;
        int32_t compute = -1;
        int32_t transfer = -1;
    };

    /** @brief Feature chain including Vulkan 1.1, 1.2, 1.3, and extended dynamic state features */
    using Features = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >;

public:
    /**
     * @brief Constructs physical device wrapper and caches capabilities
     * @param physical_device Physical device handle (moved into this object)
     */
    explicit VulkanPhysicalDevice(vk::raii::PhysicalDevice&& physical_device);

    /**
     * @brief Finds a supported depth format from common depth formats
     * @return Supported depth format (e.g., D32_SFLOAT, D24_UNORM_S8_UINT)
     */
    [[nodiscard]] vk::Format find_depth_format() const;

    /** @brief Gets driver version */
    [[nodiscard]] DriverVersion get_driver_version() const override;

    /**
     * @brief Checks if extension is supported
     * @param extensions_name Extension name to check
     * @return True if supported
     */
    [[nodiscard]] bool is_extension_supported(std::string_view extensions_name) const override;

    /**
     * @brief Checks if queue family supports presentation to surface
     * @param surface The surface
     * @param queue_family_index Queue family index to check
     * @return True if presentation is supported
     */
    [[nodiscard]] bool supports_present(Surface& surface, uint32_t queue_family_index) const override;

    /** @brief Gets the feature structure chain */
    [[nodiscard]] const Features& get_features_chain() const { return features_chain; }

    /**
     * @brief Gets format properties
     * @param format The format to query
     * @return Format properties
     */
    [[nodiscard]] vk::FormatProperties get_format_properties(vk::Format format) const;

    /** @brief Gets the physical device handle */
    [[nodiscard]] const vk::raii::PhysicalDevice& get_handle() const { return handle; }

    /** @brief Gets device features */
    [[nodiscard]] vk::PhysicalDeviceFeatures get_features() const;

    /** @brief Gets device properties */
    [[nodiscard]] vk::PhysicalDeviceProperties get_properties() const { return properties; }

    /** @brief Gets memory properties */
    [[nodiscard]] vk::PhysicalDeviceMemoryProperties get_memory_properties() const { return memory_properties; }

    /** @brief Gets queue family properties */
    [[nodiscard]] const std::vector<vk::QueueFamilyProperties>& get_queue_family_properties() const;

    /**
     * @brief Gets queue family indices for requested queue types
     * @param queue_flags Queue type flags (graphics, compute, transfer)
     * @return Queue family indices
     */
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
