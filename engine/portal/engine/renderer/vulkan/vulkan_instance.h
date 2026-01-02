//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/device/physical_device.h"
#include "portal/engine/renderer/vulkan/debug/debug_messenger.h"

namespace portal::renderer::vulkan
{
class VulkanPhysicalDevice;
}

namespace portal::renderer::vulkan
{
/** @brief Required device extensions for all platforms */
constexpr std::array REQUIRED_DEVICE_EXTENSIONS = {
    vk::KHRSwapchainExtensionName,
#if defined(PORTAL_PLATFORM_MACOS)
    vk::KHRPortabilitySubsetExtensionName
#endif

};

#ifndef PORTAL_DIST
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

/**
 * @class VulkanInstance
 * @brief Vulkan instance wrapper with debug messenger and physical device enumeration
 *
 * Creates the Vulkan instance, optional debug messenger (VK_EXT_debug_utils), and enumerates
 * all available GPUs. Provides get_suitable_gpu() to select the best device based on
 * extension support and queue family capabilities.
 */
class VulkanInstance
{
public:
    /**
     * @brief Creates Vulkan instance and debug messenger
     * @param context The vk::raii::Context (must outlive this instance)
     */
    explicit VulkanInstance(vk::raii::Context& context);

    /** @brief Gets the Vulkan instance handle */
    [[nodiscard]] const vk::raii::Instance& get_instance() const;

    /** @brief Gets the debug messenger */
    [[nodiscard]] const DebugMessenger& get_debug_messenger() const;

    /**
     * @brief Selects a suitable GPU from available physical devices
     * @return Reference to selected physical device
     */
    [[nodiscard]] VulkanPhysicalDevice& get_suitable_gpu() const;

private:
    /** @brief Enumerates and stores all physical devices */
    void query_physical_devices();

    /**
     * @brief Gets required instance extensions
     * @param enable_validation_layers Whether validation layers are enabled
     * @return Vector of extension name strings
     */
    std::vector<const char*> get_required_instance_extensions(bool enable_validation_layers);

private:
    vk::raii::Context& context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    DebugMessenger messenger;

    std::vector<std::unique_ptr<VulkanPhysicalDevice>> physical_devices;
};
} // portal
