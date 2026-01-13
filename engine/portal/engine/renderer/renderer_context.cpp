//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "renderer_context.h"

#include "portal/engine/reference.h"

namespace portal
{
RendererContext::RendererContext(
    renderer::vulkan::VulkanContext& gpu_context
) : gpu_context(gpu_context)
{
}

const renderer::vulkan::VulkanContext& RendererContext::get_gpu_context() const
{
    return gpu_context;
}

renderer::vulkan::VulkanContext& RendererContext::get_gpu_context()
{
    return gpu_context;
}

} // portal
