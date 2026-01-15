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

    void begin_frame(const FrameContext& frame);
    void end_frame(FrameContext& frame);

    void gui_update(FrameContext& frame);

private:
    const renderer::vulkan::VulkanSwapchain& swapchain;
    vk::raii::DescriptorPool imgui_pool = nullptr;
};
} // portal
