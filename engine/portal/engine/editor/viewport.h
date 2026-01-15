//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/reference.h"
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/modules/runtime_module.h"
#include "portal/engine/renderer/render_target/render_target.h"

namespace portal
{

/**
 * @brief Editor viewport that renders the scene to a separate render target displayed in ImGui.
 *
 * The Viewport class renders the scene to its own RenderTarget instead of directly to the
 * swapchain. This rendered image is then displayed as an ImGui::Image within a "Viewport"
 * window.
 */
class Viewport
{
public:
    /**
     * @brief Constructs a Viewport with initial dimensions from the swapchain.
     * @param swapchain Used to determine initial viewport size and color format.
     * @param runtime_module The runtime module that handles scene rendering.
     */
    Viewport(const renderer::vulkan::VulkanSwapchain& swapchain, RuntimeModule& runtime_module);

    ~Viewport();

    /**
     * @brief Renders the viewport image in an ImGui window.
     *
     * Handles viewport resizing when the ImGui window size changes, recreating
     * the render target and updating the descriptor set as needed.
     * @param frame The current frame context.
     */
    void on_gui_update(const FrameContext& frame);

    /**
     * @brief Renders the scene to the viewport's render target.
     * @param frame The current frame context.
     */
    void render(FrameContext& frame) const;

private:
    RuntimeModule& runtime_module;

    vk::DescriptorSet viewport_descriptor_set;
    Reference<renderer::RenderTarget> viewport_render_target;
};

} // portal
