//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_context.h"

#include <span>

#include <GLFW/glfw3.h>

#include "portal/core/log.h"
#include "portal/core/debug/assert.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/vulkan_utils.h"
#include "portal/engine/renderer/vulkan/base/allocated.h"

namespace portal::renderer::vulkan
{

constexpr std::array VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

#ifndef PORTAL_DIST
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif


const auto logger = Log::get_logger("Vulkan");

constexpr const char* get_message_type(const vk::DebugUtilsMessageTypeFlagsEXT message_type)
{
    if (message_type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
        return "General";
    if (message_type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        return "Validation";
    if (message_type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        return "Performance";
    if (message_type & vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
        return "Devive Address Binding";

    return "Unknown";
}

constexpr const char* get_severity_string(const vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        return "verbose";
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        return "info";
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        return "warning";
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        return "error";
    }

    return "unknown";
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT message_type,
    const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
    [[maybe_unused]] void* data
    )
{
    std::string labels;
    std::string objects;
    if (callback_data->cmdBufLabelCount)
    {
        labels = fmt::format("\tLabels({}): \n", callback_data->cmdBufLabelCount);
        for (uint32_t i = 0; i < callback_data->cmdBufLabelCount; ++i)
        {
            const auto& label = callback_data->pCmdBufLabels[i];
            const std::string color_str = std::format("[ {}, {}, {}, {} ]", label.color[0], label.color[1], label.color[2], label.color[3]);
            labels.append(
                std::format("\t\t- Command Buffer Label[{0}]: name: {1}, color: {2}\n", i, label.pLabelName ? label.pLabelName : "NULL", color_str)
                );
        }
    }

    if (callback_data->objectCount)
    {
        objects = fmt::format("\tObjects({}): \n", callback_data->objectCount);
        for (uint32_t i = 0; i < callback_data->objectCount; ++i)
        {
            const auto& object = callback_data->pObjects[i];
            objects.append(
                std::format(
                    "\t\t- Object[{0}] name: {1}, type: {2}, handle: {3:#x}\n",
                    i,
                    object.pObjectName ? object.pObjectName : "NULL",
                    vk::to_string(object.objectType),
                    object.objectHandle
                    )
                );
        }
    }

    LOGGER_WARN(
        "{} {} message: \n\t{}\n{} {}",
        get_message_type(message_type),
        get_severity_string(severity),
        callback_data->pMessage,
        labels,
        objects
        );
    return vk::False;
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

std::vector<const char*> get_required_instance_extensions(const vk::raii::Context& context, const bool enable_validation_layers)
{
    // ask glfw for its required extension (surface plus platform-specific extensions)
    uint32_t glfw_extension_count = 0;
    const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions = {glfw_extensions, glfw_extensions + glfw_extension_count};
#ifdef PORTAL_PLATFORM_MACOS
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif
    if (enable_validation_layers)
    {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
        extensions.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName);
    }

    if (!check_instance_extension_support(extensions, context))
        return {};

    // Trace print all available extensions
#if 0
    {
        const auto extension_props = context.enumerateInstanceExtensionProperties();
        LOG_TRACE_TAG("Renderer", "Available instance extensions:");
        for (const auto& [extensionName, specVersion] : extension_props)
        {
            std::string extension_name = extensionName;
            auto has_extension = std::ranges::find(extensions, extension_name) != extensions.end() ? "x" : " ";
            LOG_TRACE_TAG("Renderer", "  {} {}", has_extension, extension_name);
        }
    }
    LOG_TRACE_TAG("Renderer", "");
#endif

    return extensions;
}

VulkanContext::~VulkanContext()
{
    allocation::shutdown();

    PORTAL_ASSERT(device->get_ref() > 1, "Dangling reference for device");
    device = nullptr;

    PORTAL_ASSERT(physical_device->get_ref() > 1, "Dangling reference for physical device");
    physical_device = nullptr;
}

void VulkanContext::init()
{
    PORTAL_PROF_ZONE();

    LOGGER_INFO("Initializing vulkan context");
    PORTAL_ASSERT(glfwVulkanSupported(), "glfw must support vulkan");

    if (check_driver_api_version_support(vk::ApiVersion14))
    {
        PORTAL_ASSERT(false, "Incompatible vulkan driver version!");
        // TODO: exit?
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

    auto instance_extensions = get_required_instance_extensions(context, ENABLE_VALIDATION_LAYERS);
    if (instance_extensions.empty())
        PORTAL_ASSERT(false, "Incompatible instance extension!"); // TODO: exit?

    vk::ValidationFeatureEnableEXT validation_features[] = {
        vk::ValidationFeatureEnableEXT::eBestPractices
    };

    vk::ValidationFeaturesEXT validation_features_info = {
        .enabledValidationFeatureCount = static_cast<uint32_t>(std::size(validation_features)),
        .pEnabledValidationFeatures = validation_features
    };

    vk::InstanceCreateInfo instance_create_info = {
        .pNext = &validation_features_info,
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

    //TODO: call vulkan loader?

    if constexpr (ENABLE_VALIDATION_LAYERS)
    {
        constexpr auto severity_flags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        constexpr auto message_type_flags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

        constexpr vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create{
            .messageSeverity = severity_flags,
            .messageType = message_type_flags,
            .pfnUserCallback = &debug_callback,
        };
        debug_messenger = instance.createDebugUtilsMessengerEXT(debug_utils_create);
    }

    physical_device = Ref<VulkanPhysicalDevice>::create(instance);

    VulkanDevice::Features feature_chain = {
        {
            .features = {
                .independentBlend = true,
                .sampleRateShading = true,
                .fillModeNonSolid = true,
                .wideLines = true,
                .samplerAnisotropy = true,
                .pipelineStatisticsQuery = true,
                .shaderStorageImageReadWithoutFormat = true,
            }},
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

    device = Ref<VulkanDevice>::create(physical_device, feature_chain);

    allocation::init(instance, physical_device->get_handle(), device->get_handle());
}

vk::raii::Instance& VulkanContext::get_instance()
{
    return instance;
}

Ref<VulkanDevice> VulkanContext::get_device() const
{
    return device;
}

Ref<VulkanPhysicalDevice> VulkanContext::get_physical_device() const
{
    return physical_device;
}
} // portal
