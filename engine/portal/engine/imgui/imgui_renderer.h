//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/renderer.h"

namespace portal
{
class ImGuiRenderer
{
public:
    ImGuiRenderer(const Window& window, const renderer::vulkan::VulkanSwapchain& swapchain);
    ~ImGuiRenderer();

    void begin_frame(const FrameContext& frame, const Reference<renderer::RenderTarget>& render_target);
    void end_frame(FrameContext& frame);

    void gui_update(FrameContext& frame);

private:
    Reference<renderer::RenderTarget> current_render_target = nullptr;

    const renderer::vulkan::VulkanSwapchain& swapchain;
    vk::raii::DescriptorPool imgui_pool = nullptr;
};
} // portal
