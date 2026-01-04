//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_instance.h"

#include <volk.h>
#include <GLFW/glfw3.h>

#include "vulkan_utils.h"
#include "portal/core/debug/assert.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/device/vulkan_physical_device.h"

namespace portal::renderer::vulkan
{
constexpr std::array VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

const auto logger = Log::get_logger("Vulkan");

uint32_t rate_device_suitability(const VulkanPhysicalDevice& device)
{
    uint32_t score = 0;
    const auto& properties = device.get_properties();
    const auto& features = device.get_features();
    const auto& queue_families = device.get_queue_family_properties();

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

    for (const auto& extension : REQUIRED_DEVICE_EXTENSIONS)
    {
        if (!device.is_extension_supported(extension))
        {
            LOGGER_TRACE("Candidate: {} does not support extension {}", properties.deviceName.data(), extension);
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

    LOGGER_DEBUG("Gpu candidate: {} with score {}", properties.deviceName.data(), score);
    return score;
}


bool check_driver_api_version_support(const uint32_t requested_version)
{
    const uint32_t instance_version = vk::enumerateInstanceVersion();
    if (instance_version < requested_version)
    {
        LOGGER_FATAL("Incompatible vulkan driver version!");
        LOGGER_FATAL(
            "\tYou have: {}.{}.{}",
            VK_VERSION_MAJOR(instance_version),
            VK_VERSION_MINOR(instance_version),
            VK_VERSION_PATCH(instance_version)
        );
        LOGGER_FATAL(
            "\tYou need at least: {}.{}.{}",
            VK_VERSION_MAJOR(requested_version),
            VK_VERSION_MINOR(requested_version),
            VK_VERSION_PATCH(requested_version)
        );
        LOGGER_FATAL("\tPlease update your GPU driver.");

        return false;
    }

    LOGGER_TRACE(
        "Vulkan v{}.{}.{}",
        VK_VERSION_MAJOR(instance_version),
        VK_VERSION_MINOR(instance_version),
        VK_VERSION_PATCH(instance_version)
    );

    return true;
}

bool check_instance_extension_support(const std::span<const char*> extensions, const vk::raii::Context& context)
{
    auto extension_properties = context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < extensions.size(); ++i)
    {
        if (std::ranges::none_of(
            extension_properties,
            [extension=extensions[i]](const auto& property)
            {
                return strcmp(property.extensionName, extension) == 0;
            }
        ))
        {
            LOGGER_FATAL("Required Vulkan extension not supported: {}", extensions[i]);
            return false;
        }
    }

    return true;
}

bool check_validation_layer_support(const std::span<const char* const> validation_layers, const vk::raii::Context& context)
{
    auto layer_properties = context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(
        validation_layers,
        [&layer_properties](const auto& required_layer)
        {
            return std::ranges::none_of(
                layer_properties,
                [required_layer](const auto& layer_property)
                {
                    return strcmp(layer_property.layerName, required_layer) == 0;
                }
            );
        }
    ))
    {
        LOGGER_ERROR("One or more required layers are not supported!");
        return false;
    }
    return true;
}


VulkanInstance::VulkanInstance(vk::raii::Context& context) : context(context)
{
    PORTAL_PROF_ZONE();

    LOGGER_INFO("Initializing vulkan instance");
    PORTAL_ASSERT(glfwVulkanSupported(), "glfw must support vulkan");

    if (!check_driver_api_version_support(vk::ApiVersion14))
    {
        PORTAL_ASSERT(false, "Incompatible vulkan driver version!");
        // TODO: exit?
        throw std::runtime_error("Incompatible vulkan driver version!");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Application Info
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    constexpr vk::ApplicationInfo app_info = {
        .pApplicationName = "Portal Engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Portal Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Extensions and Validation
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    auto instance_extensions = get_required_instance_extensions(ENABLE_VALIDATION_LAYERS);
    PORTAL_ASSERT(!instance_extensions.empty(), "Incompatible instance extension!"); // TODO: exit?

    vk::InstanceCreateInfo instance_create_info = {
#ifdef PORTAL_PLATFORM_MACOS
        .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
        .ppEnabledExtensionNames = instance_extensions.data()
    };

    if constexpr (ENABLE_VALIDATION_LAYERS)
    {
        if (check_validation_layer_support(std::span{VALIDATION_LAYERS}, context))
        {
            vk::ValidationFeatureEnableEXT validation_features[] = {
                vk::ValidationFeatureEnableEXT::eBestPractices,
                vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
                vk::ValidationFeatureEnableEXT::eDebugPrintf
            };

            static vk::ValidationFeaturesEXT validation_features_info = {
                .enabledValidationFeatureCount = static_cast<uint32_t>(std::size(validation_features)),
                .pEnabledValidationFeatures = validation_features
            };

            instance_create_info.pNext = &validation_features_info;
            instance_create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }
        else
        {
            LOGGER_ERROR("Validation layer 'VK_LAYER_KHRONOS_validation' was not found!");
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Instance and debug messenger creation
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    instance = context.createInstance(instance_create_info);
    volkLoadInstance(*instance);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    if constexpr (ENABLE_VALIDATION_LAYERS)
    {
        constexpr auto severity_flags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        constexpr auto message_type_flags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

        const vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create{
            .messageSeverity = severity_flags,
            .messageType = message_type_flags,
            .pfnUserCallback = DebugMessenger::debug_callback,
            .pUserData = &messenger
        };
        debug_messenger = instance.createDebugUtilsMessengerEXT(debug_utils_create);
    }

    query_physical_devices();
}

const vk::raii::Instance& VulkanInstance::get_instance() const
{
    return instance;
}

const DebugMessenger& VulkanInstance::get_debug_messenger() const
{
    return messenger;
}

VulkanPhysicalDevice& VulkanInstance::get_suitable_gpu() const
{
    PORTAL_ASSERT(!physical_devices.empty(), "No physical devices found!");


    std::multimap<uint32_t, std::reference_wrapper<VulkanPhysicalDevice>> candidates;
    LOGGER_TRACE("Testing {} physical devices", physical_devices.size());
    for (const auto& dev : physical_devices)
    {
        uint32_t score = rate_device_suitability(*dev);
        candidates.emplace(score, *dev);
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0)
    {
        auto& dev = candidates.rbegin()->second.get();
        LOGGER_INFO("Picked GPU: {}", dev.get_properties().deviceName.data());
        return dev;
    }

    LOGGER_ERROR("Failed to find suitable GPU!");
    throw std::runtime_error("Failed to find suitable GPU!");
}

void VulkanInstance::query_physical_devices()
{
    const auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty())
    {
        LOGGER_ERROR("No Vulkan physical devices found!");
        throw std::runtime_error("No Vulkan physical devices found!");
    }

    // Create gpus wrapper objects from the vk::PhysicalDevice's
    for (auto device : devices)
    {
        physical_devices.emplace_back(std::make_unique<VulkanPhysicalDevice>(std::move(device)));
    }
}

std::vector<const char*> VulkanInstance::get_required_instance_extensions(const bool enable_validation_layers)
{
    // ask glfw for its required extension (surface plus platform-specific extensions)
    uint32_t glfw_extension_count = 0;
    const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    if (glfw_extension_count == 0)
        throw std::runtime_error("Failed to get required glfw extensions!, a valid vulkan driver might not be installed");

    std::vector<const char*> extensions = {glfw_extensions, glfw_extensions + glfw_extension_count};
#ifdef PORTAL_PLATFORM_MACOS
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif
    if (enable_validation_layers)
    {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    if (!check_instance_extension_support(extensions, context))
        return {};

    // Trace print all available extensions
#if 0
    {
        const auto extension_props = context.enumerateInstanceExtensionProperties();
        LOG_TRACE_TAG("Vulkan", "Available instance extensions:");
        for (const auto& [extensionName, specVersion] : extension_props)
        {
            std::string extension_name = extensionName;
            auto has_extension = std::ranges::find(extensions, extension_name) != extensions.end() ? "x" : " ";
            LOG_TRACE_TAG("Vulkan", "  {} {}", has_extension, extension_name);
        }
    }
    LOG_TRACE_TAG("Vulkan", "");
#endif

    return extensions;
}
} // portal
