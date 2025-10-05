//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "portal/core/reference.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/debug/debug_messenger.h"

namespace portal::renderer::vulkan
{

class VulkanContext final : public RefCounted
{
public:
    ~VulkanContext() override;

    void init();

    vk::raii::Instance& get_instance();
    Ref<VulkanDevice> get_device() const;
    Ref<VulkanPhysicalDevice> get_physical_device() const;

private:
    std::vector<const char*> get_required_instance_extensions(bool enable_validation_layers);

private:
    // Vulkan types
    vk::raii::Context context{};
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    std::shared_ptr<DebugMessenger> messenger;

    // Overloaded renderer types
    Ref<VulkanPhysicalDevice> physical_device;
    Ref<VulkanDevice> device;
};

} // portal