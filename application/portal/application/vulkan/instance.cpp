//
// Created by Jonatan Nevo on 02/03/2025.
//

#include "instance.h"

#include <volk.h>

#include "physical_device.h"

namespace portal::vulkan
{
#if defined(PORTAL_DEBUG)

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
)
{
    // Log debug message
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        LOG_CORE_WARN_TAG("Vulkan", "{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        LOG_CORE_ERROR_TAG("Vulkan", "{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    const VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT type,
    uint64_t object,
    size_t location,
    int32_t message_code,
    const char* layer_prefix,
    const char* message,
    void* user_data
)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        LOG_CORE_ERROR_TAG("Vulkan", "{}: {}", layer_prefix, message);
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        LOG_CORE_WARN_TAG("Vulkan", "{}: {}", layer_prefix, message);
    else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        LOG_CORE_WARN_TAG("Vulkan", "[perf] {}: {}", layer_prefix, message);
    else
        LOG_CORE_INFO_TAG("Vulkan", "{}: {}", layer_prefix, message);
    return VK_FALSE;
}
#endif

bool validate_layers(const std::vector<const char*>& required, const std::vector<vk::LayerProperties>& available)
{
    for (auto layer : required)
    {
        bool found = false;
        for (auto& available_layer : available)
        {
            if (strcmp(available_layer.layerName, layer) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            LOG_CORE_ERROR_TAG("Vulkan", "Validation Layer {} not found", layer);
            return false;
        }
    }
    return true;
}

bool enable_extension(
    const char* requested_extension,
    const std::vector<vk::ExtensionProperties>& available_extensions,
    std::vector<const char*>& enabled_extensions
)
{
    const bool is_available = std::ranges::any_of(
        available_extensions,
        [&requested_extension](const auto& available_extension) { return strcmp(requested_extension, available_extension.extensionName) == 0; }
    );

    if (is_available)
    {
        const bool is_already_enabled = std::ranges::any_of(
            enabled_extensions,
            [&requested_extension](const auto& enabled_extension) { return strcmp(requested_extension, enabled_extension) == 0; }
        );
        if (!is_already_enabled)
        {
            LOG_CORE_INFO_TAG("Vulkan", "Extension {} available, enabling it", requested_extension);
            enabled_extensions.push_back(requested_extension);
        }
    }
    else
    {
        LOG_CORE_WARN_TAG("Vulkan", "Extension {} is not available", requested_extension);
    }

    return is_available;
}

bool enable_layer(
    const char* requested_layer,
    const std::vector<vk::LayerProperties>& available_layers,
    std::vector<const char*>& enabled_layers
)
{
    const bool is_available = std::ranges::any_of(
        available_layers,
        [&requested_layer](const auto& available_layer) { return strcmp(requested_layer, available_layer.layerName) == 0; }
    );

    if (is_available)
    {
        const bool is_already_enabled = std::ranges::any_of(
            enabled_layers,
            [&requested_layer](const auto& enabled_layer) { return strcmp(requested_layer, enabled_layer) == 0; }
        );
        if (!is_already_enabled)
        {
            LOG_CORE_INFO_TAG("Vulkan", "Layer {} available, enabling it", requested_layer);
            enabled_layers.push_back(requested_layer);
        }
    }
    else
    {
        LOG_CORE_WARN_TAG("Vulkan", "Extension {} is not available", requested_layer);
    }

    return is_available;
}

Instance::Instance(
    const std::string& application_name,
    const std::unordered_map<const char*, bool>& requested_extensions,
    const std::unordered_map<const char*, bool>& requested_layers,
    const std::vector<vk::LayerSettingEXT>& required_layer_settings,
    uint32_t api_version
)
{
    const auto available_instance_extensions = vk::enumerateInstanceExtensionProperties();

#if defined(PORTAL_DEBUG)
    // Check if VK_EXT_debug_utils is supported, which supersedes VK_EXT_Debug_Report
    const bool has_debug_utils = enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, available_instance_extensions, enabled_extensions);
    bool has_debug_report = false;

    if (!has_debug_utils)
    {
        has_debug_report = enable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, available_instance_extensions, enabled_extensions);
        if (!has_debug_report)
            LOG_CORE_WARN_TAG(
            "Vulkan",
            "Neither of {} or {} are available; disabling debug reporting",
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME
        );
    }

    bool validation_features = false;
    {
        const auto available_layer_instance_extensions = vk::enumerateInstanceExtensionProperties(std::string("VK_LAYER_KHRONOS_validation"));
        enable_extension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME, available_layer_instance_extensions, enabled_extensions);
    }
#endif

    // Specific surface extensions are obtained from  Window::get_required_surface_extensions
    // They are already added to requested_extensions by Renderer::on_start

    // Even for a headless surface a swapchain is still required
    enable_extension(VK_KHR_SURFACE_EXTENSION_NAME, available_instance_extensions, enabled_extensions);

    // VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
    // which will be used for stats gathering where available.
    enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, available_instance_extensions, enabled_extensions);

    for (const auto& [extension_name, extension_is_optional] : requested_extensions)
    {
        if (!enable_extension(extension_name, available_instance_extensions, enabled_extensions))
        {
            if (extension_is_optional)
                LOG_CORE_WARN_TAG("Vulkan", "Optional instance extension {} not available, some features may be disabled", extension_name);
            else
            {
                LOG_CORE_ERROR_TAG("Vulkan", "Required instance extension {} not available, cannot run", extension_name);
                throw std::runtime_error("Required instance extension not available");
            }
        }
    }

    const auto supported_layers = vk::enumerateInstanceLayerProperties();
    std::vector<const char*> enabled_layers;

    for (const auto& [layer_name, layer_is_optional] : requested_layers)
    {
        if (!enable_layer(layer_name, supported_layers, enabled_layers))
        {
            if (layer_is_optional)
                LOG_CORE_WARN_TAG("Vulkan", "Optional layer {} not available, some features may be disabled", layer_name);
            else
            {
                LOG_CORE_ERROR_TAG("Vulkan", "Required layer {} not available, cannot run", layer_name);
                throw std::runtime_error("Required layer not available");
            }
        }
    }

    enable_layer("VK_LAYER_KHRONOS_validation", supported_layers, enabled_layers);

    const vk::ApplicationInfo app_info(application_name.c_str(), 0, "Portal Engine", 0, api_version);
    vk::InstanceCreateInfo instance_info({}, &app_info, enabled_layers, enabled_extensions);

#if defined(PORTAL_DEBUG)
    vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info;
    vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;

    if (has_debug_utils)
    {
        debug_utils_create_info = vk::DebugUtilsMessengerCreateInfoEXT(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debug_utils_messenger_callback
        );

        instance_info.pNext = &debug_utils_create_info;
    }
    else if (has_debug_report)
    {
        debug_report_create_info = vk::DebugReportCallbackCreateInfoEXT(
            vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning,
            debug_callback
        );

        instance_info.pNext = &debug_report_create_info;
    }
#endif

    vk::LayerSettingsCreateInfoEXT layerSettingsCreateInfo;

    // If layer settings are defined, then activate the required layer settings during instance creation
    if (!required_layer_settings.empty())
    {
        layerSettingsCreateInfo.settingCount = static_cast<uint32_t>(required_layer_settings.size());
        layerSettingsCreateInfo.pSettings = required_layer_settings.data();
        layerSettingsCreateInfo.pNext = instance_info.pNext;
        instance_info.pNext = &layerSettingsCreateInfo;
    }

    // Create the Vulkan instance
    handle = vk::createInstance(instance_info);

    // initialize the Vulkan-Hpp default dispatcher on the instance
    VULKAN_HPP_DEFAULT_DISPATCHER.init(handle);

    // Need to load volk for all the not-yet Vulkan-Hpp calls
    volkLoadInstance(handle);

#if defined(PORTAL_DEBUG)
    if (has_debug_utils)
        debug_utils_messenger = handle.createDebugUtilsMessengerEXT(debug_utils_create_info);
    else if (has_debug_report)
        debug_report_callback = handle.createDebugReportCallbackEXT(debug_report_create_info);
#endif

    query_gpus();
}

Instance::Instance(vk::Instance instance): handle(instance)
{
    if (handle)
        query_gpus();
    else
        throw std::runtime_error("Instance not valid");
}

Instance::~Instance()
{
#if defined(PORTAL_DEBUG)
    if (debug_utils_messenger)
        handle.destroyDebugUtilsMessengerEXT(debug_utils_messenger);
    if (debug_report_callback)
        handle.destroyDebugReportCallbackEXT(debug_report_callback);
#endif

    if (handle)
        handle.destroy();
}

const std::vector<const char*>& Instance::get_extensions() { return enabled_extensions; }

bool Instance::is_enabled(const char* extension) const
{
    return std::ranges::find_if(
        enabled_extensions,
        [extension](const char* enabled) { return strcmp(extension, extension) == 0; }
    ) != enabled_extensions.end();
}

vk::Instance Instance::get_handle() const
{
    return handle;
}

PhysicalDevice& Instance::get_first_gpu()
{
    PORTAL_CORE_ASSERT(!gpus.empty(), "No physical devices were found on the system.");

    // Find a discrete GPU
    for (const auto& gpu : gpus)
    {
        if (gpu->get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            return *gpu;
    }

    // Otherwise just pick the first one
    LOG_CORE_WARN_TAG("Vulkan", "No discrete physical device found, picking default GPU");
    return *gpus[0];
}

PhysicalDevice& Instance::get_suitable_gpu(vk::SurfaceKHR surface, bool headless_surface)
{
    PORTAL_CORE_ASSERT(!gpus.empty(), "No physical devices were found on the system.");

    for (const auto& gpu : gpus)
    {
        if (gpu->get_properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            // See if it works with the surface
            const size_t queue_count = gpu->get_queue_family_properties().size();
            for (uint32_t queue_idx = 0; static_cast<size_t>(queue_idx) < queue_count; queue_idx++)
            {
                if (gpu->get_handle().getSurfaceSupportKHR(queue_idx, surface))
                {
                    return *gpu;
                }
            }
        }
    }

    // Otherwise just pick the first one
    LOG_CORE_WARN_TAG("Vulkan", "No discrete physical device found, picking default GPU");
    return *gpus[0];
}

void Instance::query_gpus()
{
    auto physical_devices = handle.enumeratePhysicalDevices();
    if (physical_devices.empty())
        throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");

    for (auto& physical_device : physical_devices)
    {
        gpus.push_back(std::make_unique<PhysicalDevice>(*this, physical_device));
    }
}
} // portal
