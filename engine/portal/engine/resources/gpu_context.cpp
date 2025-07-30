//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "gpu_context.h"

#include "portal/core/log.h"
#include "portal/engine/renderer/allocated_buffer.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/renderer/vulkan_utils.h"

namespace portal::resources
{
static auto logger = Log::get_logger("Resources");

GpuContext::GpuContext(vk::raii::Device& device, vk::raii::CommandBuffer& commandBuffer, vk::raii::Queue& submit_queue)
    : device(device),
      command_buffer(commandBuffer),
      submit_queue(submit_queue)
{
    if (device != nullptr)
        fence = device.createFence(vk::FenceCreateInfo{});
}

void GpuContext::immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function) const
{
    device.resetFences({fence});
    command_buffer.reset();

    command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    function(command_buffer);
    command_buffer.end();

    vk::CommandBufferSubmitInfo cmd_submit_info{
        .commandBuffer = command_buffer,
        .deviceMask = 0,
    };

    vk::SubmitInfo2 submit_info{
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_submit_info
    };

    submit_queue.submit2({submit_info}, fence);
    const auto result = device.waitForFences({fence}, true, std::numeric_limits<uint64_t>::max());
    if (result != vk::Result::eSuccess)
    {
        LOGGER_ERROR("Failed to wait for immediate command buffer submission: {}", vk::to_string(result));
    }
}

vulkan::AllocatedImage GpuContext::create_image(void* data, vulkan::ImageBuilder image_builder) const
{
    auto image = image_builder.build(device);
    populate_image(data, image);
    return image;
}

void GpuContext::populate_image(const void* data, vulkan::AllocatedImage& image) const
{
    const size_t data_size = image.get_extent().width * image.get_extent().height * image.get_extent().depth * 4;
    auto staging_buffer = vulkan::AllocatedBuffer::create_staging_buffer(device, data_size, data);

    immediate_submit(
        [&](const auto& command_buffer)
        {
            // TODO: move these to some `command buffer` class and use it directly
            vulkan::transition_image_layout(
                command_buffer,
                image.get_handle(),
                image.get_mip_levels(),
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal
                );

            vk::BufferImageCopy copy_region = {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .imageExtent = image.get_extent(),
            };

            command_buffer.copyBufferToImage(
                staging_buffer.get_handle(),
                image.get_handle(),
                vk::ImageLayout::eTransferDstOptimal,
                {copy_region}
                );

            if (image.get_mip_levels() > 1)
                generate_mipmaps(command_buffer, image);
        }
        );
}

void GpuContext::generate_mipmaps(const vk::raii::CommandBuffer& active_cmd, const vulkan::AllocatedImage& image) const
{
    // TODO: contain the physical device inside the device class so we can access it here
    // const auto format_properties = physical_device.getFormatProperties(image.get_format());
    // if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    // {
    //     throw std::runtime_error("Texture image format does not support linear blitting!");
    // }

    int32_t mip_width = image.get_extent().width;
    int32_t mip_height = image.get_extent().height;

    vk::ImageSubresourceRange subresource_range = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };

    for (uint32_t i = 1; i < image.get_mip_levels(); ++i)
    {
        subresource_range.baseMipLevel = i - 1;
        vulkan::transition_image_layout(
            active_cmd,
            image.get_handle(),
            subresource_range,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::AccessFlagBits2::eTransferWrite,
            vk::AccessFlagBits2::eTransferRead,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::PipelineStageFlagBits2::eTransfer
            );;


        vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dst_offsets;
        offsets[0] = vk::Offset3D(0, 0, 0);
        offsets[1] = vk::Offset3D(mip_width, mip_height, 1);
        dst_offsets[0] = vk::Offset3D(0, 0, 0);
        dst_offsets[1] = vk::Offset3D(mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1);
        vk::ImageBlit blit = {
            .srcSubresource = {vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
            .srcOffsets = offsets,
            .dstSubresource = {vk::ImageAspectFlagBits::eColor, i, 0, 1},
            .dstOffsets = dst_offsets
        };
        active_cmd.blitImage(
            image.get_handle(),
            vk::ImageLayout::eTransferSrcOptimal,
            image.get_handle(),
            vk::ImageLayout::eTransferDstOptimal,
            {blit},
            vk::Filter::eLinear
            );

        vulkan::transition_image_layout(
            active_cmd,
            image.get_handle(),
            subresource_range,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::AccessFlagBits2::eTransferRead,
            vk::AccessFlagBits2::eShaderRead,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::PipelineStageFlagBits2::eFragmentShader
            );
        if (mip_width > 1)
            mip_width /= 2;
        if (mip_height > 1)
            mip_height /= 2;
    }

    subresource_range.baseMipLevel = image.get_mip_levels() - 1;
    vulkan::transition_image_layout(
        active_cmd,
        image.get_handle(),
        subresource_range,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite,
        vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::PipelineStageFlagBits2::eFragmentShader
        );
}

}
