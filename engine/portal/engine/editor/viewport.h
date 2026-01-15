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
class Viewport
{
public:
    Viewport(const renderer::vulkan::VulkanSwapchain& swapchain, RuntimeModule& runtime_module);
    ~Viewport();

    void on_gui_update(const FrameContext& frame);
    void render(FrameContext& frame) const;

private:
    RuntimeModule& runtime_module;

    vk::DescriptorSet viewport_descriptor_set;
    Reference<renderer::RenderTarget> viewport_render_target;
};
} // portal
