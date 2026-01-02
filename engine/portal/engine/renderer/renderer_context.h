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

/**
 * @class RendererContext
 * @brief Shared rendering context accessible during frame rendering
 *
 * Provides access to Vulkan context, global descriptor set layouts, and attachment properties.
 * Passed to systems/modules that need GPU resources during rendering.
 */
class RendererContext
{
public:
    /**
     * @brief Constructs renderer context
     * @param gpu_context Vulkan context
     * @param global_descriptor_set_layout Global descriptor set layouts
     * @param attachments Attachment properties (color/depth formats)
     */
    RendererContext(
        renderer::vulkan::VulkanContext& gpu_context,
        std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout,
        renderer::AttachmentProperties& attachments
    );

    /** @brief Gets Vulkan context (const) */
    [[nodiscard]] const renderer::vulkan::VulkanContext& get_gpu_context() const;

    /** @brief Gets Vulkan context (mutable) */
    [[nodiscard]] renderer::vulkan::VulkanContext& get_gpu_context();

    /** @brief Gets attachment properties */
    [[nodiscard]] const renderer::AttachmentProperties& get_attachments() const;

    /**
     * @brief Gets global descriptor set layouts as range view
     * @return Range view of descriptor set layouts
     */
    [[nodiscard]] auto get_global_descriptor_set_layout() const
    {
        return global_descriptor_set_layout | std::ranges::views::transform([](const auto& layout) { return *layout; });
    }

public:
    size_t frames_in_flight = 0;

protected:
    renderer::vulkan::VulkanContext& gpu_context;
    std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout;
    renderer::AttachmentProperties& attachments;
};
} // portal
