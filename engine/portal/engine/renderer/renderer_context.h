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
namespace renderer::vulkan
{
    class VulkanSwapchain;
}

class RendererContext
{
public:
    RendererContext(
        renderer::vulkan::VulkanContext& gpu_context,
        std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout,
        renderer::AttachmentProperties& attachments
    );

    [[nodiscard]] const renderer::vulkan::VulkanContext& get_gpu_context() const;
    [[nodiscard]] renderer::vulkan::VulkanContext& get_gpu_context();
    [[nodiscard]] const renderer::AttachmentProperties& get_attachments() const;

    [[nodiscard]] auto get_global_descriptor_set_layout() const
    {
        return global_descriptor_set_layout | std::ranges::views::transform([](const auto& layout) { return *layout; });
    }

public:
    size_t frames_in_flight = 0;

protected:
    // TODO: use baseclasses here
    renderer::vulkan::VulkanContext& gpu_context;
    std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout;
    renderer::AttachmentProperties& attachments;
};
} // portal
