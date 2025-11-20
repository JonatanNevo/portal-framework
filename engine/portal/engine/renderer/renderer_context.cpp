//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "renderer_context.h"

#include "portal/engine/reference.h"

namespace portal
{
RendererContext::RendererContext(
    renderer::vulkan::VulkanContext& gpu_context,
    std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout,
    const renderer::AttachmentProperties& attachments
) : gpu_context(gpu_context),
    global_descriptor_set_layout(global_descriptor_set_layout),
    attachments(attachments)
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

const renderer::AttachmentProperties& RendererContext::get_attachments() const
{
    return attachments;
}
} // portal
