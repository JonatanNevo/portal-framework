//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <volk.h>
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_instance.h"
#include "device/vulkan_physical_device.h"

namespace portal::renderer::vulkan
{
/**
 * @class VulkanContext
 * @brief Top-level owner of the Vulkan object hierarchy
 *
 * Owns and initializes the complete Vulkan object lifetime chain:
 * Context → Instance → PhysicalDevice → Device → VMA.
 *
 * Constructor sequence:
 * 1. Creates vk::raii::Context (loads Vulkan functions)
 * 2. Creates VulkanInstance (creates instance and debug messenger)
 * 3. Selects suitable GPU via instance.get_suitable_gpu()
 * 4. Creates VulkanDevice from selected physical device
 * 5. Initializes VMA allocator
 *
 * Destructor ensures correct destruction order: VMA shutdown first,
 * then device/physical_device/instance in reverse construction order.
 */
class VulkanContext final
{
public:
    /** @brief Initializes Vulkan object hierarchy and VMA */
    static std::unique_ptr<VulkanContext> create();

    /** @brief Shuts down VMA and destroys Vulkan objects in reverse order */
    ~VulkanContext();

    /** @brief Gets the Vulkan instance */
    [[nodiscard]] const vk::raii::Instance& get_instance() const;

    /** @brief Gets the logical device (const) */
    [[nodiscard]] const VulkanDevice& get_device() const;

    /** @brief Gets the logical device (mutable) */
    VulkanDevice& get_device();

    /** @brief Gets the selected physical device (GPU) */
    [[nodiscard]] const VulkanPhysicalDevice& get_physical_device() const;

private:
    VulkanContext();

    vk::raii::Context context;
    VulkanInstance instance;

    VulkanPhysicalDevice& physical_device;
    VulkanDevice device;
};
} // portal
