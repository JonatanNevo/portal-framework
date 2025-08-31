//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "portal/core/reference.h"

namespace portal::renderer::vulkan
{

class VulkanDevice;
class VulkanPhysicalDevice;

class VulkanContext final : public RefCounted
{
public:
    ~VulkanContext() override;

    void init();

    vk::raii::Instance& get_instance();
    Ref<VulkanDevice> get_device() const;
    Ref<VulkanPhysicalDevice> get_physical_device() const;

private:
    // Vulkan types
    vk::raii::Context context{};
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;

    // Overloaded renderer types
    Ref<VulkanPhysicalDevice> physical_device;
    Ref<VulkanDevice> device;
};

} // portal