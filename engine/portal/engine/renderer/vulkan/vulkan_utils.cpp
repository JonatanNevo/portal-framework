//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_utils.h"

#include <GLFW/glfw3.h>

#include "image/vulkan_image.h"
#include "portal/core/log.h"

namespace portal::renderer::vulkan
{
class VulkanImage;
static auto logger = Log::get_logger("Renderer");

vk::SampleCountFlagBits get_max_usable_sample_count(vk::raii::PhysicalDevice& physical_device)
{
    const auto device_properties = physical_device.getProperties();

    const auto counts = device_properties.limits.framebufferColorSampleCounts & device_properties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }
    return vk::SampleCountFlagBits::e1;
}

void image_barrier(
    const vk::CommandBuffer& command_buffer,
    const vk::Image& image,
    const vk::ImageSubresourceRange& subresource,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::AccessFlags2 src_access_mask,
    const vk::PipelineStageFlags2 dst_stage_mask,
    const vk::AccessFlags2 dst_access_mask,
    const bool discard_contents
)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = discard_contents ? vk::ImageLayout::eUndefined : vk::ImageLayout::eGeneral,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresource
    };

    const vk::DependencyInfo dependency_info = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    command_buffer.pipelineBarrier2(dependency_info);
}

void image_barrier(
    const vk::CommandBuffer& command_buffer,
    const Reference<Image>& image,
    const uint32_t mip_count,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::AccessFlags2 src_access_mask,
    const vk::PipelineStageFlags2 dst_stage_mask,
    const vk::AccessFlags2 dst_access_mask,
    const bool discard_contents,
    const vk::ImageAspectFlags aspect_mask
)
{
    const vk::ImageSubresourceRange subresource_range{
        .aspectMask = aspect_mask,
        .baseMipLevel = 0,
        .levelCount = mip_count,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers
    };

    image_barrier(
        command_buffer,
        reference_cast<VulkanImage>(image)->get_image().get_handle(),
        subresource_range,
        src_stage_mask,
        src_access_mask,
        dst_stage_mask,
        dst_access_mask,
        discard_contents
    );
}
}
