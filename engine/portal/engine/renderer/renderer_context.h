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
 * Provides access to Vulkan context, and attachment properties.
 * Passed to systems/modules that need GPU resources during rendering.
 */
class RendererContext
{
public:
    /**
     * @brief Constructs renderer context
     * @param gpu_context Vulkan context
     */
    RendererContext(
        renderer::vulkan::VulkanContext& gpu_context
    );

    /** @brief Gets Vulkan context (const) */
    [[nodiscard]] const renderer::vulkan::VulkanContext& get_gpu_context() const;

    /** @brief Gets Vulkan context (mutable) */
    [[nodiscard]] renderer::vulkan::VulkanContext& get_gpu_context();

public:
    size_t frames_in_flight = 0;

protected:
    renderer::vulkan::VulkanContext& gpu_context;
};
} // portal
