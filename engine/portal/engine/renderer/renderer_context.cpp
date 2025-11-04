//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "renderer_context.h"

#include <__msvc_ranges_to.hpp>

#include "portal/engine/reference.h"

namespace portal
{
RendererContext::RendererContext(
    renderer::vulkan::VulkanContext& gpu_context,
    Reference<renderer::RenderTarget>& render_target,
    std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout
    ) : gpu_context(gpu_context),
        render_target(render_target),
        global_descriptor_set_layout(global_descriptor_set_layout) {
}

const renderer::vulkan::VulkanContext& RendererContext::get_gpu_context() const
{
    return gpu_context;
}

renderer::vulkan::VulkanContext& RendererContext::get_gpu_context()
{
    return gpu_context;
}

const Reference<renderer::RenderTarget>& RendererContext::get_render_target() const
{
    return render_target;
}

Reference<renderer::RenderTarget>& RendererContext::get_render_target()
{
    return render_target;
}
} // portal
