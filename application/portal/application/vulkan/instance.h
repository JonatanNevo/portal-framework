//
// Created by Jonatan Nevo on 02/03/2025.
//

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace portal::vulkan
{
class PhysicalDevice;

/**
 * @brief A wrapper class for vk::Instance
 *
 * This class is responsible for initializing the dispatcher, enumerating over all available extensions and validation layers
 * enabling them if they exist, setting up debug messaging and querying all the physical devices existing on the machine.
 */
class Instance
{
public:
    /**
     * @brief Initializes the connection to Vulkan
     * @param application_name The name of the application
     * @param requested_extensions The extensions requested to be enabled
     * @param requested_layers The validation layers to be enabled
     * @param required_layer_settings The layer settings to be enabled
     * @param api_version The Vulkan API version that the instance will be using
     * @throws runtime_error if the required extensions and validation layers are not found
     */
    Instance(
        const std::string& application_name,
        const std::unordered_map<const char*, bool>& requested_extensions = {},
        const std::unordered_map<const char*, bool>& requested_layers = {},
        const std::vector<vk::LayerSettingEXT>& required_layer_settings = {},
        uint32_t api_version = VK_API_VERSION_1_0
    );

    /**
     * @brief Queries the GPUs of a vk::Instance that is already created
     * @param instance A valid vk::Instance
     */
    Instance(vk::Instance instance);

    /**
     * @brief Destroys the debug utils and the handle
     */
    ~Instance();

    Instance(const Instance&) = delete;
    Instance(Instance&&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance& operator=(Instance&&) = delete;

    /**
     * @return The enabled extensions on this instance
     */
    const std::vector<const char*>& get_extensions();

    /**
     * @brief Checks if the given extension is enabled in the vk::Instance
     * @param extension An extension to check
     */
    bool is_enabled(const char* extension) const;

    /**
     * @brief Tries to find the first available discrete GPU
     * @returns A valid physical device
     */
    PhysicalDevice& get_first_gpu();

    /**
     * @brief Tries to find the first available discrete GPU that can render to the given surface
     * @param surface to test against
     * @param headless_surface Is surface created with VK_EXT_headless_surface
     * @returns A valid physical device
     */
    PhysicalDevice& get_suitable_gpu(vk::SurfaceKHR surface, bool headless_surface);

    /**
     * @return The underlying vk::Instance handle of the instance
     */
    vk::Instance get_handle() const;

private:
    /**
     * @brief Queries the instance for the physical devices on the machine
     */
    void query_gpus();

private:
    /**
     * @brief The Vulkan instance
     */
    vk::Instance handle;

    /**
     * @brief The enabled extensions
     */
    std::vector<const char *> enabled_extensions;

#if defined(PORTAL_DEBUG)
    /**
     * @brief Debug utils messenger callback for VK_EXT_Debug_Utils
     */
    vk::DebugUtilsMessengerEXT debug_utils_messenger;

    /**
     * @brief The debug report callback
     */
    vk::DebugReportCallbackEXT debug_report_callback;
#endif

    /**
     * @brief The physical devices found on the machine
     */
    std::vector<std::unique_ptr<PhysicalDevice>> gpus;
};
} // portal
