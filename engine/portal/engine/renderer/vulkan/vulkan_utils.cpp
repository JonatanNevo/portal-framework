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

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    const uint32_t mip_level,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout
)
{
    vk::PipelineStageFlags2 source_stage;
    vk::PipelineStageFlags2 destination_stage;
    vk::AccessFlags2 src_access_mask;
    vk::AccessFlags2 dst_access_mask;

    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
    {
        src_access_mask = {};
        dst_access_mask = vk::AccessFlagBits2::eTransferWrite;

        source_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits2::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        src_access_mask = vk::AccessFlagBits2::eTransferWrite;
        dst_access_mask = vk::AccessFlagBits2::eShaderRead;

        source_stage = vk::PipelineStageFlagBits2::eTransfer;
        destination_stage = vk::PipelineStageFlagBits2::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    transition_image_layout(
        command_buffer,
        image,
        mip_level,
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        source_stage,
        destination_stage,
        vk::ImageAspectFlagBits::eColor
    );
}

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const Reference<Image>& image,
    const uint32_t mip_level,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout
)
{
    transition_image_layout(
        command_buffer,
        reference_cast<VulkanImage>(image)->get_image().get_handle(),
        mip_level,
        old_layout,
        new_layout
    );
}

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    const uint32_t mip_level,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask,
    const vk::ImageAspectFlags aspect_mask
)
{
    const vk::ImageSubresourceRange subresource_range{
        .aspectMask = aspect_mask,
        .baseMipLevel = 0,
        .levelCount = mip_level,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers
    };

    transition_image_layout(
        command_buffer,
        image,
        subresource_range,
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        src_stage_mask,
        dst_stage_mask
    );
}

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const Reference<Image>& image,
    const uint32_t mip_level,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask,
    const vk::ImageAspectFlags aspect_mask
)
{
    transition_image_layout(
        command_buffer,
        reference_cast<VulkanImage>(image)->get_image().get_handle(),
        mip_level,
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        src_stage_mask,
        dst_stage_mask,
        aspect_mask
    );
}

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    const vk::ImageSubresourceRange& subresource,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask
)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
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

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const Reference<Image>& image,
    const vk::ImageSubresourceRange& subresource,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask
)
{
    transition_image_layout(
        command_buffer,
        reference_cast<VulkanImage>(image)->get_image().get_handle(),
        subresource,
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        src_stage_mask,
        dst_stage_mask
    );
}

void copy_image_to_image(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& source,
    const vk::Image& dest,
    const vk::Extent2D src_size,
    const vk::Extent2D dst_size
)
{
    // Calculate aspect ratios
    const float src_aspect = static_cast<float>(src_size.width) / static_cast<float>(src_size.height);
    const float dst_aspect = static_cast<float>(dst_size.width) / static_cast<float>(dst_size.height);

    // Calculate scaled destination rectangle maintaining aspect ratio
    auto scaled_width = static_cast<int32_t>(dst_size.width);
    auto scaled_height = static_cast<int32_t>(dst_size.height);
    int32_t offset_x = 0;
    int32_t offset_y = 0;

    if (src_aspect > dst_aspect)
    {
        // Source is wider - add letterboxing (black bars on top/bottom)
        scaled_height = static_cast<int32_t>(static_cast<float>(dst_size.width) / src_aspect);
        offset_y = (static_cast<int32_t>(dst_size.height) - scaled_height) / 2;
    }
    else if (src_aspect < dst_aspect)
    {
        // Source is taller - add pillarboxing (black bars on left/right)
        scaled_width = static_cast<int32_t>(static_cast<float>(dst_size.height) * src_aspect);
        offset_x = (static_cast<int32_t>(dst_size.width) - scaled_width) / 2;
    }

    vk::ImageBlit2 blit_region{
        .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .srcOffsets = std::array{vk::Offset3D{}, vk::Offset3D{static_cast<int32_t>(src_size.width), static_cast<int32_t>(src_size.height), 1}},
        .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .dstOffsets = std::array{vk::Offset3D{offset_x, offset_y, 0}, vk::Offset3D{offset_x + scaled_width, offset_y + scaled_height, 1}}
    };

    const vk::BlitImageInfo2 blit_info = {
        .srcImage = source,
        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
        .dstImage = dest,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = &blit_region,
        .filter = vk::Filter::eLinear
    };

    command_buffer.blitImage2(blit_info);
}
}
