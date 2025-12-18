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
#include "device/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/vulkan_utils.h"
#include "portal/engine/renderer/vulkan/base/allocated.h"
#include "portal/engine/renderer/vulkan/debug/debug_messenger.h"

namespace portal::renderer::vulkan
{
const auto logger = Log::get_logger("Vulkan");


VulkanContext::VulkanContext() :
    instance(context),
    physical_device(instance.get_suitable_gpu()),
    device(physical_device, physical_device.get_features_chain())
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

VulkanDevice& VulkanContext::get_device()
{
    return device;
}

const VulkanPhysicalDevice& VulkanContext::get_physical_device() const
{
    return physical_device;
}
} // portal
