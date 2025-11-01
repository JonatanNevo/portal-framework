//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/vulkan/debug/debug_messenger.h"

namespace portal::renderer::vulkan
{

class VulkanInstance
{
public:
    explicit VulkanInstance(vk::raii::Context& context);

    [[nodiscard]] const vk::raii::Instance& get_instance() const;
    [[nodiscard]] const DebugMessenger& get_debug_messenger() const;

private:
    std::vector<const char*> get_required_instance_extensions(bool enable_validation_layers);

private:
    vk::raii::Context& context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    DebugMessenger messenger;
};

} // portal