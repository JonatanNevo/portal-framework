//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_context.h"

#include <span>

#include <GLFW/glfw3.h>
#include <steam/steamtypes.h>

#include "portal/core/log.h"
#include "portal/core/debug/assert.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/vulkan_utils.h"
#include "portal/engine/renderer/vulkan/base/allocated.h"
#include "portal/engine/renderer/vulkan/debug/debug_messenger.h"

namespace portal::renderer::vulkan
{

const auto logger = Log::get_logger("Vulkan");

VulkanDevice::Features prepare_device_features(const VulkanPhysicalDevice& physical_device)
{
    const auto available_features = physical_device.get_features();

    VulkanDevice::Features feature_chain = {
        {
            .features = {
                .independentBlend = available_features.independentBlend,
                .sampleRateShading = available_features.sampleRateShading,
                .fillModeNonSolid = available_features.fillModeNonSolid,
                .wideLines = available_features.wideLines,
                .samplerAnisotropy = available_features.samplerAnisotropy,
                .pipelineStatisticsQuery = available_features.pipelineStatisticsQuery,
                .shaderStorageImageReadWithoutFormat = available_features.shaderStorageImageReadWithoutFormat,
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

    return feature_chain;
}

VulkanContext::VulkanContext() :
    instance(context),
    physical_device(instance.get_instance()),
    device(physical_device, prepare_device_features(physical_device))
{
    allocation::init(instance.get_instance(), physical_device.get_handle(), device.get_handle());
}

VulkanContext::~VulkanContext()
{
    allocation::shutdown();
}

const vk::raii::Instance& VulkanContext::get_instance() const
{
    return instance.get_instance();
}

const VulkanDevice& VulkanContext::get_device() const
{
    return device;
}

const VulkanPhysicalDevice& VulkanContext::get_physical_device() const
{
    return physical_device;
}
} // portal
