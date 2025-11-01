//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_instance.h"
#include "portal/engine/renderer/vulkan/vulkan_physical_device.h"

namespace portal::renderer::vulkan
{

class VulkanContext final
{
public:
    VulkanContext();
    ~VulkanContext();

    const vk::raii::Instance& get_instance() const;
    const VulkanDevice& get_device() const;
    const VulkanPhysicalDevice& get_physical_device() const;

private:
    // Vulkan types
    vk::raii::Context context{};
    VulkanInstance instance;

    // Overloaded renderer types
    VulkanPhysicalDevice physical_device;
    VulkanDevice device;
};

} // portal