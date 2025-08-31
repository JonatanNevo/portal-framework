//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "gpu_context.h"

#include "portal/core/log.h"
#include "allocated_buffer.h"
#include "vulkan_image.h"
#include "portal/engine/renderer/descriptor_writer.h"
#include "vulkan_utils.h"
#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/renderer/vulkan/pipeline_builder.h"
#include "portal/engine/renderer/vulkan/vulkan_common.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_physical_device.h"

namespace portal::renderer::vulkan
{

GpuContext::GpuContext(
    vk::raii::Device& device,
    const Ref<RenderTarget>& render_target,
    const std::vector<vk::DescriptorSetLayout>& global_descriptor_layouts
    ): render_target(render_target),
        global_descriptor_layouts(global_descriptor_layouts),
        device(device)
{
}

vk::Device GpuContext::get_device() const
{
    return device;
}

Ref<RenderTarget> GpuContext::get_render_target() const
{
    return render_target;
}

vk::raii::DescriptorSet GpuContext::create_descriptor_set(const vk::DescriptorSetLayout& layout)
{
    return descriptor_allocator.allocate(layout);
}


std::vector<vk::DescriptorSetLayout>& GpuContext::get_global_descriptor_layouts()
{
    return global_descriptor_layouts;
}

void GpuContext::write_descriptor_set(portal::vulkan::DescriptorWriter& writer, vk::raii::DescriptorSet& set)
{
    writer.update_set(device, set);
}

// void GpuContext::populate_image(const void* data, AllocatedImage& image, const vk::Extent3D extent, size_t mip_level) const
// {
//     const size_t data_size = extent.width * extent.height * extent.depth * 4;
//     auto staging_buffer = renderer::vulkan::AllocatedBuffer::create_staging_buffer(context->get_device(), data_size, data);
//
//     immediate_submit(
//         [&](const auto& command_buffer)
//         {
//             // TODO: move these to some `command buffer` class and use it directly
//             portal::vulkan::transition_image_layout(
//                 command_buffer,
//                 image.get_handle(),
//                 mip_level,
//                 vk::ImageLayout::eUndefined,
//                 vk::ImageLayout::eTransferDstOptimal
//                 );
//
//             vk::BufferImageCopy copy_region = {
//                 .bufferOffset = 0,
//                 .bufferRowLength = 0,
//                 .bufferImageHeight = 0,
//                 .imageSubresource = {
//                     .aspectMask = vk::ImageAspectFlagBits::eColor,
//                     .mipLevel = 0,
//                     .baseArrayLayer = 0,
//                     .layerCount = 1
//                 },
//                 .imageExtent = extent,
//             };
//
//             command_buffer.copyBufferToImage(
//                 staging_buffer.get_handle(),
//                 image.get_handle(),
//                 vk::ImageLayout::eTransferDstOptimal,
//                 {copy_region}
//                 );
//
//             if (mip_level > 1)
//                 generate_mipmaps(command_buffer, image);
//             else
//                 portal::vulkan::transition_image_layout(
//                     command_buffer,
//                     image.get_handle(),
//                     mip_level,
//                     vk::ImageLayout::eTransferDstOptimal,
//                     vk::ImageLayout::eShaderReadOnlyOptimal
//                     );
//         }
//         );
// }
//
// void GpuContext::generate_mipmaps(const vk::raii::CommandBuffer& active_cmd, const Ref<VulkanImage>& image) const
// {
//     // TODO: contain the physical device inside the device class so we can access it here
//     const auto format_properties = context->get_physical_device()->get_handle().getFormatProperties(image->get_format());
//     if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
//     {
//         throw std::runtime_error("Texture image format does not support linear blitting!");
//     }
//
//     int32_t mip_width = image->get_width();
//     int32_t mip_height = image->get_height();
//
//     vk::ImageSubresourceRange subresource_range = {
//         .aspectMask = vk::ImageAspectFlagBits::eColor,
//         .baseMipLevel = 0,
//         .levelCount = 1,
//         .baseArrayLayer = 0,
//         .layerCount = 1
//     };
//
//     for (uint32_t i = 1; i < image->mip; ++i)
//     {
//         subresource_range.baseMipLevel = i - 1;
//         portal::vulkan::transition_image_layout(
//             active_cmd,
//             image.get_handle(),
//             subresource_range,
//             vk::ImageLayout::eTransferDstOptimal,
//             vk::ImageLayout::eTransferSrcOptimal,
//             vk::AccessFlagBits2::eTransferWrite,
//             vk::AccessFlagBits2::eTransferRead,
//             vk::PipelineStageFlagBits2::eTransfer,
//             vk::PipelineStageFlagBits2::eTransfer
//             );;
//
//
//         vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dst_offsets;
//         offsets[0] = vk::Offset3D(0, 0, 0);
//         offsets[1] = vk::Offset3D(mip_width, mip_height, 1);
//         dst_offsets[0] = vk::Offset3D(0, 0, 0);
//         dst_offsets[1] = vk::Offset3D(mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1);
//         vk::ImageBlit blit = {
//             .srcSubresource = {vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
//             .srcOffsets = offsets,
//             .dstSubresource = {vk::ImageAspectFlagBits::eColor, i, 0, 1},
//             .dstOffsets = dst_offsets
//         };
//         active_cmd.blitImage(
//             image.get_handle(),
//             vk::ImageLayout::eTransferSrcOptimal,
//             image.get_handle(),
//             vk::ImageLayout::eTransferDstOptimal,
//             {blit},
//             vk::Filter::eLinear
//             );
//
//         portal::vulkan::transition_image_layout(
//             active_cmd,
//             image.get_handle(),
//             subresource_range,
//             vk::ImageLayout::eTransferSrcOptimal,
//             vk::ImageLayout::eShaderReadOnlyOptimal,
//             vk::AccessFlagBits2::eTransferRead,
//             vk::AccessFlagBits2::eShaderRead,
//             vk::PipelineStageFlagBits2::eTransfer,
//             vk::PipelineStageFlagBits2::eFragmentShader
//             );
//         if (mip_width > 1)
//             mip_width /= 2;
//         if (mip_height > 1)
//             mip_height /= 2;
//     }
//
//     subresource_range.baseMipLevel = image.get_mip_levels() - 1;
//     portal::vulkan::transition_image_layout(
//         active_cmd,
//         image.get_handle(),
//         subresource_range,
//         vk::ImageLayout::eTransferDstOptimal,
//         vk::ImageLayout::eShaderReadOnlyOptimal,
//         vk::AccessFlagBits2::eTransferWrite,
//         vk::AccessFlagBits2::eShaderRead,
//         vk::PipelineStageFlagBits2::eTransfer,
//         vk::PipelineStageFlagBits2::eFragmentShader
//         );
// }

}
