//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"

namespace portal::renderer::vulkan
{
class VulkanSwapchainRenderTarget: public VulkanRenderTarget
{
public:
    explicit VulkanSwapchainRenderTarget(const RenderTargetProperties& prop, VulkanSwapchain& swapchain);

    void begin_frame(FrameContext& frame) override;
    void end_frame(const FrameContext& frame) override;

    void resize(size_t new_width, size_t new_height, bool force_recreate) override;

protected:
    void inner_initialize() override;
    void inner_release() override;

    void make_depth_image();

private:
    VulkanSwapchain& swapchain;

    // TODO: Should this be here or moved to some `g buffer` class
    Reference<VulkanImage> depth_image;
};

} // portal