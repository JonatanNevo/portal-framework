//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/render_target/render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"

#include <ranges>

namespace portal
{
namespace renderer::vulkan {
    class VulkanSwapchain;
}

class RendererContext
{
public:
    RendererContext(
        renderer::vulkan::VulkanContext& gpu_context,
        std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout,
        const Reference<renderer::vulkan::VulkanSwapchain>& swapchain
        );


    [[nodiscard]] const renderer::vulkan::VulkanContext& get_gpu_context() const;
    [[nodiscard]] renderer::vulkan::VulkanContext& get_gpu_context();

    [[nodiscard]] auto get_global_descriptor_set_layout() const
    {
        return global_descriptor_set_layout | std::ranges::views::transform([](const auto& layout) { return *layout; });
    }

    [[nodiscard]] auto get_swapchain() const { return swapchain; }

protected:
    // TODO: use baseclasses here
    renderer::vulkan::VulkanContext& gpu_context;
    Reference<renderer::vulkan::VulkanSwapchain> swapchain;
    std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout;
};

} // portal