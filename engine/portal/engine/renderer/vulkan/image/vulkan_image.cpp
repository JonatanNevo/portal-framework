//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_image.h"
#include <vulkan/vulkan_format_traits.hpp>

#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_utils.h"
#include "portal/engine/renderer/vulkan/image/vulkan_sampler.h"

namespace portal::renderer::vulkan
{
static auto logger = Log::get_logger("Vulkan");

VulkanImage::VulkanImage(const vk::Image image, const image::Properties& properties, const VulkanContext& context)
    : Image(properties.name),
      context(context),
      device(context.get_device()),
      properties(properties)
{
    // TODO: move this to the `reallocate` somehow?
    image_info.image = ImageAllocation(image);

    ImageViewProperties imageViewProperties{
        .image = this,
        .mip = 0,
        .name = STRING_ID(fmt::format("{}_view", properties.name.string))
    };
    image_info.view = make_reference<VulkanImageView>(imageViewProperties, context);
}

VulkanImage::VulkanImage(const vk::Image image, vk::ImageView image_view, const image::Properties& properties, const VulkanContext& context)
    : Image(properties.name),
      context(context),
      device(context.get_device()),
      properties(properties)
{
    image_info.image = ImageAllocation(image);

    ImageViewProperties imageViewProperties{
        .image = this,
        .mip = 0,
        .name = STRING_ID(fmt::format("{}_view", properties.name.string))
    };
    image_info.view = make_reference<VulkanImageView>(image_view, imageViewProperties, context);
}

VulkanImage::VulkanImage(const image::Properties& properties, const VulkanContext& context)
    : Image(properties.name),
      context(context),
      device(context.get_device()),
      properties(properties)
{
    PORTAL_ASSERT(properties.width > 0 && properties.height > 0, "Invalid image size");
    reallocate();
}

VulkanImage::~VulkanImage()
{
    release();
}

void VulkanImage::resize(const size_t width, const size_t height)
{
    properties.width = width;
    properties.height = height;
    reallocate();
}

void VulkanImage::reallocate()
{
    if (image_info.image.get_handle() != nullptr)
    {
        device.wait_idle();
    }
    release();

    ImageBuilder builder(properties.width, properties.height, 1);

    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
    if (properties.usage == ImageUsage::Attachment || properties.usage == ImageUsage::SubAttachment)
    {
        if (utils::is_depth_format(properties.format))
            usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        else
            usage |= vk::ImageUsageFlagBits::eColorAttachment;
    }
    if (properties.transfer || properties.usage == ImageUsage::Texture)
    {
        usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
    }
    if (properties.usage == ImageUsage::Storage)
    {
        usage |= vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst;
    }

    vk::ImageAspectFlags aspect_mask = utils::is_depth_format(properties.format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    if (utils::is_stencil_format(properties.format))
        aspect_mask |= vk::ImageAspectFlagBits::eStencil;

    const vk::Format format = to_format(properties.format);

    const VmaMemoryUsage memory_usage = properties.usage == ImageUsage::HostRead ? VMA_MEMORY_USAGE_GPU_TO_CPU : VMA_MEMORY_USAGE_GPU_ONLY;

    builder.with_usage(usage)
           .with_image_type(vk::ImageType::e2D)
           .with_format(format)
           .with_mips_levels(properties.mips)
           .with_array_layers(properties.layers)
           .with_sample_count(vk::SampleCountFlagBits::e1)
           .with_tiling(properties.usage == ImageUsage::HostRead ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal)
           .with_vma_usage(memory_usage)
           .with_vma_required_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
           .with_debug_name(std::string(properties.name.string));

    if (properties.flags == image::Flags::CubeCompatible)
        builder.with_flags(vk::ImageCreateFlagBits::eCubeCompatible);

    image_info.image = device.create_image(builder);

    ImageViewProperties imageViewProperties{
        .image = this,
        .name = STRING_ID(fmt::format("{}_view", properties.name.string))
    };
    image_info.view = make_reference<VulkanImageView>(imageViewProperties, context);

    if (properties.create_sampler)
    {
        SamplerProperties sampler_prop{
            .wrap = TextureWrap::Clamp,
        };

        if (utils::is_integer_format(properties.format))
        {
            sampler_prop.filter = TextureFilter::Nearest;
            sampler_prop.mipmap_mode = SamplerMipmapMode::Nearest;
        }
        else
        {
            sampler_prop.filter = TextureFilter::Linear;
            sampler_prop.mipmap_mode = SamplerMipmapMode::Linear;
        }

        image_info.sampler = make_reference<VulkanSampler>(
            STRING_ID(fmt::format("default_sampler_{}", properties.name.string)),
            sampler_prop,
            device
        );
    }

    if (properties.usage == ImageUsage::Storage)
    {
        device.immediate_submit(
            [&](const auto& command_buffer)
            {
                vk::ImageSubresourceRange range{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = static_cast<uint32_t>(properties.mips),
                    .baseArrayLayer = 0,
                    .layerCount = static_cast<uint32_t>(properties.layers)
                };

                vulkan::transition_image_layout(
                    command_buffer,
                    image_info.image.get_handle(),
                    range,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    vk::AccessFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eAllCommands,
                    vk::PipelineStageFlagBits2::eAllCommands
                );
            }
        );
    }

    if (properties.usage == ImageUsage::HostRead)
    {
        device.immediate_submit(
            [&](const auto& command_buffer)
            {
                vk::ImageSubresourceRange range{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = static_cast<uint32_t>(properties.mips),
                    .baseArrayLayer = 0,
                    .layerCount = static_cast<uint32_t>(properties.layers)
                };

                vulkan::transition_image_layout(
                    command_buffer,
                    image_info.image.get_handle(),
                    range,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::AccessFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eAllCommands,
                    vk::PipelineStageFlagBits2::eAllCommands
                );
            }
        );
    }
}

void VulkanImage::release()
{
    if (image_info.image.get_handle() == nullptr)
        return;

    // Submit async free?
    per_layer_image_views.clear();
    per_mip_image_views.clear();

    image_info.image = nullptr;
    image_info.sampler = nullptr;
    image_info.view = nullptr;
}

bool VulkanImage::is_image_valid() const
{
    return descriptor_image_info.imageView != nullptr;
}

size_t VulkanImage::get_width() const
{
    return properties.width;
}

size_t VulkanImage::get_height() const
{
    return properties.height;
}

glm::uvec2 VulkanImage::get_size() const
{
    return {properties.width, properties.height};
}

vk::Format VulkanImage::get_format() const
{
    return to_format(properties.format);
}

bool VulkanImage::has_mip() const
{
    return properties.mips > 1;
}

float VulkanImage::get_aspect_ratio() const
{
    return static_cast<float>(properties.width) / static_cast<float>(properties.height);
}

int VulkanImage::get_closest_mip_level(const size_t width, const size_t height) const
{
    if (width > properties.width / 2 || height > properties.height / 2)
        return 0;

    const int a = static_cast<int>(glm::log2(glm::min(static_cast<float>(properties.width), static_cast<float>(properties.height))));
    const int b = static_cast<int>(glm::log2(glm::min(static_cast<float>(width), static_cast<float>(height))));
    return a - b;
}

std::pair<size_t, size_t> VulkanImage::get_mip_level_dimensions(const size_t mip_level) const
{
    return {properties.width >> mip_level, properties.height >> mip_level};
}

image::Properties& VulkanImage::get_prop()
{
    return properties;
}

const image::Properties& VulkanImage::get_prop() const
{
    return properties;
}

void VulkanImage::create_per_layer_image_view()
{
    PORTAL_ASSERT(properties.layers > 1, "Cannot create per layer image view for single layer image");

    vk::ImageAspectFlags aspect_mask = utils::is_depth_format(properties.format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    if (utils::is_stencil_format(properties.format))
        aspect_mask |= vk::ImageAspectFlagBits::eStencil;

    per_layer_image_views.reserve(properties.layers);
    for (size_t layer = 0; layer < properties.layers; ++layer)
    {
        ImageViewProperties imageViewProperties{
            .image = this,
            .layer = layer,
            .name = STRING_ID(fmt::format("{}_layer_view_{}", properties.name.string, layer))
        };
        per_layer_image_views.emplace_back(make_reference<VulkanImageView>(imageViewProperties, context));
    }
}

Reference<VulkanImageView> VulkanImage::get_mip_image_view(size_t mip_level)
{
    if (!per_mip_image_views.contains(mip_level))
    {
        ImageViewProperties imageViewProperties{
            .image = this,
            .mip = mip_level,
            .name = STRING_ID(fmt::format("{}_mip_view_{}", properties.name.string, mip_level))
        };
        per_mip_image_views.emplace(mip_level, make_reference<VulkanImageView>(imageViewProperties, context));
    }
    return per_mip_image_views.at(mip_level);
}

Reference<VulkanImageView> VulkanImage::get_layer_image_view(const size_t layer)
{
    PORTAL_ASSERT(layer < properties.layers, "Invalid layer index");
    return per_layer_image_views[layer];
}

VulkanImageInfo& VulkanImage::get_image_info()
{
    return image_info;
}

const vk::DescriptorImageInfo& VulkanImage::get_descriptor_image_info() const
{
    return descriptor_image_info;
}

const ImageAllocation& VulkanImage::get_image() const
{
    return image_info.image;
}

Reference<ImageView> VulkanImage::get_view() const
{
    return image_info.view;
}

const Reference<VulkanSampler>& VulkanImage::get_sampler() const
{
    return image_info.sampler;
}

Buffer VulkanImage::get_buffer() const
{
    return image_data;
}

Buffer& VulkanImage::get_buffer()
{
    return image_data;
}

void VulkanImage::set_data(const Buffer buffer)
{
    PORTAL_ASSERT(properties.transfer, "Image was not created with transfer bit");

    if (!buffer)
    {
        LOGGER_WARN("Attempting to write an empty buffer");
        return;
    }

    auto staging_buffer = AllocatedBuffer::create_staging_buffer(device, buffer.size, buffer.data);

    device.immediate_submit(
        [&](const auto& command_buffer)
        {
            vk::ImageSubresourceRange range{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            };

            portal::renderer::vulkan::transition_image_layout(
                command_buffer,
                image_info.image.get_handle(),
                range,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::AccessFlagBits2::eNone,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eHost,
                vk::PipelineStageFlagBits2::eTransfer
            );

            vk::BufferImageCopy copy_region{
                .bufferOffset = 0,
                .imageSubresource = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .imageExtent = {
                    static_cast<uint32_t>(properties.width),
                    static_cast<uint32_t>(properties.height),
                    1
                }
            };

            command_buffer.copyBufferToImage(
                staging_buffer.get_handle(),
                image_info.image.get_handle(),
                vk::ImageLayout::eTransferDstOptimal,
                {copy_region}
            );

            portal::renderer::vulkan::transition_image_layout(
                command_buffer,
                image_info.image.get_handle(),
                range,
                vk::ImageLayout::eTransferDstOptimal,
                descriptor_image_info.imageLayout,
                vk::AccessFlagBits2::eTransferRead,
                vk::AccessFlagBits2::eShaderRead,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::PipelineStageFlagBits2::eFragmentShader
            );
        }
    );

    update_descriptor();
}

portal::Buffer VulkanImage::copy_to_host_buffer()
{
    const size_t buffer_size = utils::get_image_memory_size(properties.format, properties.width, properties.height);

    BufferBuilder builder(buffer_size);
    builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
           .with_usage(vk::BufferUsageFlagBits::eTransferDst)
           .with_sharing_mode(vk::SharingMode::eExclusive)
           .with_vma_usage(VMA_MEMORY_USAGE_GPU_TO_CPU)
           .with_debug_name("staging");

    AllocatedBuffer staging_buffer = device.create_buffer(builder);

    // TODO: support copy of mips?
    constexpr uint32_t mip_count = 1;
    auto mip_width = static_cast<uint32_t>(properties.width);
    auto mip_height = static_cast<uint32_t>(properties.height);

    device.immediate_submit(
        [&](const vk::raii::CommandBuffer& command_buffer)
        {
            constexpr vk::ImageSubresourceRange range{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = mip_count,
                .baseArrayLayer = 0,
                .layerCount = 1
            };

            portal::renderer::vulkan::transition_image_layout(
                command_buffer,
                image_info.image.get_handle(),
                range,
                descriptor_image_info.imageLayout,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::AccessFlagBits2::eNone,
                vk::AccessFlagBits2::eTransferRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eTransfer
            );

            size_t mip_data_offset = 0;
            for (uint32_t mip = 0; mip < mip_count; mip++)
            {
                vk::BufferImageCopy copy_region{
                    .bufferImageHeight = static_cast<uint32_t>(mip_data_offset),
                    .imageSubresource = {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .mipLevel = mip,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .imageExtent = {
                        mip_width,
                        mip_height,
                        1
                    }
                };

                command_buffer.copyImageToBuffer(
                    image_info.image.get_handle(),
                    vk::ImageLayout::eTransferSrcOptimal,
                    staging_buffer.get_handle(),
                    {copy_region}
                );

                mip_data_offset += utils::get_image_memory_size(properties.format, mip_width, mip_height);
                mip_width = std::max(1u, mip_width / 2);
                mip_height = std::max(1u, mip_height / 2);
            }

            portal::renderer::vulkan::transition_image_layout(
                command_buffer,
                image_info.image.get_handle(),
                range,
                vk::ImageLayout::eTransferSrcOptimal,
                descriptor_image_info.imageLayout,
                vk::AccessFlagBits2::eTransferRead,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::PipelineStageFlagBits2::eTopOfPipe
            );
        }
    );

    const auto mapped_memory = staging_buffer.map();

    return Buffer::copy(mapped_memory, buffer_size);
}

void VulkanImage::update_descriptor()
{
    if (utils::is_depth_format(properties.format))
        descriptor_image_info.imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    else if (properties.usage == ImageUsage::Storage)
        descriptor_image_info.imageLayout = vk::ImageLayout::eGeneral;
    else if (properties.usage == ImageUsage::HostRead)
        descriptor_image_info.imageLayout = vk::ImageLayout::eTransferDstOptimal;
    else if (properties.usage == ImageUsage::Attachment)
        descriptor_image_info.imageLayout = vk::ImageLayout::ePresentSrcKHR;
    else
        descriptor_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    descriptor_image_info.imageView = image_info.view->get_vk_image_view();
    if (image_info.sampler)
        descriptor_image_info.sampler = image_info.sampler->get_vk_sampler();
}

VulkanImageView::VulkanImageView(const vk::ImageView image_view, const ImageViewProperties& image_view_properties, const VulkanContext& context)
    : ImageView(image_view_properties),
      context(context),
      image_view(image_view),
      owner(false)
{
}

VulkanImageView::VulkanImageView(const ImageViewProperties& image_view_properties, const VulkanContext& context)
    : ImageView(image_view_properties),
      context(context),
      owner(true)
{
    const auto vulkan_image = dynamic_cast<VulkanImage*>(image_view_properties.image);
    const auto image_properties = vulkan_image->get_prop();

    vk::ImageAspectFlags aspect_mask = utils::is_depth_format(image_properties.format)
                                           ? vk::ImageAspectFlagBits::eDepth
                                           : vk::ImageAspectFlagBits::eColor;
    if (utils::is_stencil_format(image_properties.format))
        aspect_mask |= vk::ImageAspectFlagBits::eStencil;

    const vk::ImageViewCreateInfo view_info{
        .image = vulkan_image->get_image().get_handle(),
        .viewType = image_properties.layers > 1 ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D,
        .format = to_format(image_properties.format),
        .subresourceRange = {
            .aspectMask = aspect_mask,
            .baseMipLevel = static_cast<uint32_t>(properties.mip),
            .levelCount = static_cast<uint32_t>(image_properties.mips),
            .baseArrayLayer = static_cast<uint32_t>(properties.layer),
            .layerCount = static_cast<uint32_t>(image_properties.layers)
        }
    };

    image_view = context.get_device().create_image_view(view_info);
    context.get_device().set_debug_name(image_view, properties.name.string.data());
}

VulkanImageView::~VulkanImageView()
{
    if (owner)
    {
        context.get_device().destory_image_view(image_view);
    }
}

vk::ImageView VulkanImageView::get_vk_image_view() const
{
    return image_view;
}
}
