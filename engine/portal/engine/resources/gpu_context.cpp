//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "gpu_context.h"

#include "portal/core/log.h"
#include "portal/engine/renderer/allocated_buffer.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/renderer/descriptor_writer.h"
#include "portal/engine/renderer/vulkan_utils.h"

namespace portal::resources
{
static auto logger = Log::get_logger("Resources");

#define VK_HANDLE_CAST(raii_obj) reinterpret_cast<uint64_t>(static_cast<decltype(raii_obj)::CType>(*(raii_obj)))


vk::ShaderStageFlagBits to_vk_shader_stage(const ShaderStage stage)
{
#define CASE(FROM, TO)             \
case portal::ShaderStage::FROM:    \
return vk::ShaderStageFlagBits::TO

    switch (stage)
    {
    CASE(All, eAll);
    CASE(Vertex, eVertex);
    CASE(Fragment, eFragment);
    CASE(Geometry, eGeometry);
    CASE(Compute, eCompute);
    CASE(RayGeneration, eRaygenKHR);
    CASE(Intersection, eIntersectionKHR);
    CASE(AnyHit, eAnyHitKHR);
    CASE(ClosestHit, eClosestHitKHR);
    CASE(Miss, eMissKHR);
    CASE(Callable, eCallableKHR);
    CASE(Mesh, eMeshEXT);
    }

#undef CASE
    return vk::ShaderStageFlagBits::eAll;
}

GpuContext::GpuContext(
    vk::raii::Device& device,
    vk::raii::CommandBuffer& commandBuffer,
    vk::raii::Queue& submit_queue,
    vulkan::AllocatedImage& draw_image,
    vulkan::AllocatedImage& depth_image,
    const std::vector<vk::DescriptorSetLayout>& global_descriptor_layouts
    ) : global_descriptor_layouts(global_descriptor_layouts),
        device(device),
        command_buffer(commandBuffer),
        submit_queue(submit_queue),
        draw_image(draw_image),
        depth_image(depth_image)
{
    if (device != nullptr)
    {
        fence = device.createFence(vk::FenceCreateInfo{});
        device.setDebugUtilsObjectNameEXT(
            {
                .objectType = vk::ObjectType::eFence,
                .objectHandle = VK_HANDLE_CAST(fence),
                .pObjectName = "Immediate buffer fence"
            }
            );

        // TODO: move somewhere
        std::vector<vulkan::DescriptorAllocator::PoolSizeRatio> sizes = {
            {vk::DescriptorType::eCombinedImageSampler, 3},
            {vk::DescriptorType::eUniformBuffer, 3},
            {vk::DescriptorType::eStorageBuffer, 1}
        };
        descriptor_allocator.init(&device, 16, sizes);
    }
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


vulkan::AllocatedBuffer GpuContext::create_buffer(vulkan::BufferBuilder builder) const
{
    return builder.build(device);
}

std::shared_ptr<vulkan::AllocatedBuffer> GpuContext::create_buffer_shared(vulkan::BufferBuilder builder) const
{
    return builder.build_shared(device);
}

vulkan::AllocatedImage GpuContext::create_image(void* data, vulkan::ImageBuilder image_builder) const
{
    auto image = image_builder.build(device);
    populate_image(data, image);
    return image;
}

std::shared_ptr<vulkan::AllocatedImage> GpuContext::create_image_shared(void* data, vulkan::ImageBuilder image_builder) const
{
    auto image = image_builder.build_shared(device);
    populate_image(data, *image);
    return image;
}

vk::raii::Sampler GpuContext::create_sampler(const vk::SamplerCreateInfo create_info) const
{
    return device.createSampler(create_info);
}

vk::raii::DescriptorSetLayout GpuContext::create_descriptor_set_layout(vulkan::DescriptorLayoutBuilder& builder)
{
    return builder.build(device);
}

vk::raii::DescriptorSet GpuContext::create_descriptor_set(const vk::DescriptorSetLayout& layout)
{
    return descriptor_allocator.allocate(layout);
}

vk::raii::PipelineLayout GpuContext::create_pipeline_layout(const vk::PipelineLayoutCreateInfo& pipeline_layout_info)
{
    return device.createPipelineLayout(pipeline_layout_info);
}

vk::raii::ShaderModule GpuContext::create_shader_module(const Buffer code)
{
    const vk::ShaderModuleCreateInfo shader_module_create_info{
        .codeSize = code.size * sizeof(char),
        .pCode = code.as<uint32_t*>()
    };

    return device.createShaderModule(shader_module_create_info);
}

vk::raii::Pipeline GpuContext::create_pipeline(vulkan::PipelineBuilder builder)
{
    return builder.build(device);
}

std::vector<vk::DescriptorSetLayout>& GpuContext::get_global_descriptor_layouts()
{
    return global_descriptor_layouts;
}

void GpuContext::write_descriptor_sets(Ref<Shader> shader, std::vector<vk::raii::DescriptorSet>& sets, const size_t skip)
{
    PORTAL_ASSERT(shader->descriptor_writers.size() - skip == sets.size(), "Number of descriptor sets does not match number of descriptor writers");

    size_t skipped = 0;
    for (size_t i = 0; i < shader->descriptor_writers.size(); ++i)
    {
        if (skipped < skip)
        {
            skipped++;
            continue;
        }

        shader->descriptor_writers[i].update_set(device, sets[i - skip]);
    }
}

vk::Format GpuContext::get_draw_image_format()
{
    return draw_image.get_format();
}

vk::Format GpuContext::get_depth_format()
{
    return depth_image.get_format();
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
            else
                vulkan::transition_image_layout(
                    command_buffer,
                    image.get_handle(),
                    image.get_mip_levels(),
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal
                    );
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
