//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/renderer.h"

namespace portal
{

/**
 * @brief Manages ImGui rendering integration with Vulkan.
 *
 * ImGuiRenderer handles ImGui initialization, frame management, and rendering
 * to a specified render target. Used by EditorModule to render the editor UI.
 */
class ImGuiRenderer
{
public:
    /**
     * @brief Initializes ImGui with Vulkan backend.
     * @param window The window for input handling.
     * @param swapchain The swapchain for format information.
     */
    ImGuiRenderer(const Window& window, const renderer::vulkan::VulkanSwapchain& swapchain);

    ~ImGuiRenderer();

    /**
     * @brief Begins an ImGui frame.
     * @param frame The current frame context.
     * @param render_target The render target to draw ImGui to.
     */
    void begin_frame(const FrameContext& frame, const Reference<renderer::RenderTarget>& render_target);

    /**
     * @brief Ends the ImGui frame and records draw commands.
     * @param frame The current frame context.
     */
    void end_frame(FrameContext& frame);

private:
    Reference<renderer::RenderTarget> current_render_target = nullptr;

    const renderer::vulkan::VulkanSwapchain& swapchain;
    vk::raii::DescriptorPool imgui_pool = nullptr;
};

} // portal
