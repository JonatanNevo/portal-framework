//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_instance.h"
#include "device/vulkan_physical_device.h"

namespace portal::renderer::vulkan
{
class VulkanContext final
{
public:
    VulkanContext();
    ~VulkanContext();

    const vk::raii::Instance& get_instance() const;
    const VulkanDevice& get_device() const;
    VulkanDevice& get_device();
    const VulkanPhysicalDevice& get_physical_device() const;

private:
    vk::raii::Context context{};
    VulkanInstance instance;

    VulkanPhysicalDevice& physical_device;
    VulkanDevice device;
};
} // portal
