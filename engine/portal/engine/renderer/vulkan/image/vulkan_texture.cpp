//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_texture.h"
#include "portal/engine/renderer/image/image.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_utils.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/engine/renderer/vulkan/image/vulkan_sampler.h"

namespace portal::renderer::vulkan
{

namespace utils
{
    bool validate_spec(const TextureSpecification& spec)
    {
        bool result = true;
        result = spec.width > 0 && spec.height > 0 && spec.depth > 0 && spec.width < std::numeric_limits<uint32_t>::max() && spec.height <
            std::numeric_limits<uint32_t>::max() && spec.depth < std::numeric_limits<uint32_t>::max();
        PORTAL_ASSERT(result, "Invalid texture specification");

        return result;
    }

    vk::Filter to_filter(const TextureFilter filter)
    {
        switch (filter)
        {
        case TextureFilter::Linear:
            return vk::Filter::eLinear;
        case TextureFilter::Nearest:
            return vk::Filter::eNearest;
        case TextureFilter::Cubic:
            return vk::Filter::eCubicIMG;
        default:
            break;
        }
        PORTAL_ASSERT(false, "Unsupported texture filter");
        return vk::Filter::eLinear;
    }

    vk::SamplerAddressMode to_address_mode(const TextureWrap wrap)
    {
        switch (wrap)
        {
        case TextureWrap::Clamp:
            return vk::SamplerAddressMode::eClampToEdge;
        case TextureWrap::Repeat:
            return vk::SamplerAddressMode::eRepeat;

        default:
            break;
        }

        PORTAL_ASSERT(false, "Unsupported texture wrap");
        return vk::SamplerAddressMode::eClampToEdge;
    }

    vk::ImageViewType get_view_type(const TextureSpecification& spec)
    {
        if (spec.width >= 1)
        {
            if (spec.height >= 1)
            {
                if (spec.depth > 1)
                {
                    return vk::ImageViewType::e3D;
                }
                return vk::ImageViewType::e2D;
            }
            return vk::ImageViewType::e1D;
        }

        PORTAL_ASSERT(false, "Unsupported texture specification");
        return vk::ImageViewType::e2D;
    }
}


VulkanTexture::VulkanTexture(
    const StringId& id,
    const TextureSpecification& spec,
    const Buffer& data,
    const VulkanContext& context
    ) : Texture(id), spec(spec), context(context), device(context.get_device())
{
    utils::validate_spec(spec);

    if (data)
    {
        image_data = Buffer::copy(data.data, data.size);
    }
    else
    {
        const auto size = renderer::utils::get_image_memory_size(spec.format, spec.width, spec.height, spec.depth);
        image_data = Buffer::allocate(size);
        image_data.zero_initialize();
    }

    recreate();
}

void VulkanTexture::resize(const glm::uvec3& size)
{
    return resize(size.x, size.y, size.z);
}

void VulkanTexture::resize(const size_t width, const size_t height, const size_t depth)
{
    spec.width = width;
    spec.height = height;
    spec.depth = depth;

    recreate();
}

void VulkanTexture::update_image()
{
    set_data(image_data);
}

void VulkanTexture::set_sampler(const Reference<Sampler>& sampler)
{
    const auto vulkan_sampler = reference_cast<VulkanSampler>(sampler);
    PORTAL_ASSERT(vulkan_sampler, "Invalid sampler type");

    spec.sampler_spec = vulkan_sampler->get_spec();

    auto& info = image->get_image_info();
    info.sampler = vulkan_sampler;
    image->update_descriptor();
}

ImageFormat VulkanTexture::get_format() const
{
    return spec.format;
}

size_t VulkanTexture::get_width() const
{
    return spec.width;
}

size_t VulkanTexture::get_height() const
{
    return spec.height;
}

size_t VulkanTexture::get_depth() const
{
    return spec.depth;
}

glm::uvec3 VulkanTexture::get_size() const
{
    return {spec.width, spec.height, spec.depth};
}

uint32_t VulkanTexture::get_mip_level_count() const
{
    return static_cast<uint32_t>(renderer::utils::calculate_mip_count(spec.width, spec.height, spec.depth));
}

glm::uvec3 VulkanTexture::get_mip_size(const uint32_t mip) const
{
    return {spec.width >> mip, spec.height >> mip, spec.depth >> mip};
}

Reference<Image> VulkanTexture::get_image() const
{
    return image;
}

Buffer VulkanTexture::get_writeable_buffer()
{
    return image_data;
}

TextureType VulkanTexture::get_type() const
{
    return spec.type;
}

const vk::DescriptorImageInfo& VulkanTexture::get_descriptor_image_info() const
{
    return image->get_descriptor_image_info();
}

bool VulkanTexture::loaded() const
{
    return image != nullptr;
}

void VulkanTexture::recreate()
{
    if (image != nullptr)
        image.reset();

    const auto mip_count = spec.generate_mipmaps ? get_mip_level_count() : 1;
    const auto layer_count = get_array_layer_count();

    image::Specification image_spec{
        .format = spec.format,
        .width = spec.width,
        .height = spec.height,
        .depth = spec.depth,
        .mips = mip_count,
        .layers = layer_count,
        .create_sampler = false,
        .name = id,
    };

    if (spec.storage)
        image_spec.usage = ImageUsage::Storage;
    image = make_reference<VulkanImage>(image_spec, context);
    image->update_descriptor();

    auto& info = image->get_image_info();
    if (image_data)
    {
        set_data(image_data);
    }
    else
    {
        device.immediate_submit(
            [&](const vk::raii::CommandBuffer& command_buffer)
            {
                vk::ImageSubresourceRange range{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = mip_count,
                    .baseArrayLayer = 0,
                    .layerCount = layer_count
                };

                portal::renderer::vulkan::transition_image_layout(
                    command_buffer,
                    info.image.get_handle(),
                    range,
                    vk::ImageLayout::eUndefined,
                    image->get_descriptor_image_info().imageLayout,
                    vk::AccessFlagBits2::eNone,
                    vk::AccessFlagBits2::eShaderRead,
                    vk::PipelineStageFlagBits2::eAllCommands,
                    vk::PipelineStageFlagBits2::eAllCommands
                    );
            }
            );
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CREATE TEXTURE SAMPLER (owned by Image)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (spec.sampler_spec.has_value())
    {
        auto& sampler_spec = spec.sampler_spec.value();
        info.sampler = make_reference<VulkanSampler>(STRING_ID(std::format("{}-sampler", id)), sampler_spec, device);
        image->update_descriptor();
    }

    if (!spec.storage)
    {
        const vk::ImageViewCreateInfo view_info{
            .image = info.image.get_handle(),
            .viewType = utils::get_view_type(spec),
            .format = to_format(spec.format),
            .components = {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
            // The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
            // It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = mip_count,
                .baseArrayLayer = 0,
                .layerCount = layer_count
            },
        };
        info.view = device.create_image_view(view_info);
        device.set_debug_name(info.view, std::format("texture_view_{}", id.string).c_str());
        image->update_descriptor();
    }
}

void VulkanTexture::set_data(const Buffer& data)
{
    auto& info = image->get_image_info();

    auto staging_buffer = AllocatedBuffer::create_staging_buffer(device, data.size, data.data);

    const uint32_t layer_count = get_array_layer_count();

    device.immediate_submit(
        [&](const vk::raii::CommandBuffer& command_buffer)
        {
            // The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
            vk::ImageSubresourceRange range{
                // Image only contains color data
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                // Start at the first mip level
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = layer_count
            };

            // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
            portal::renderer::vulkan::transition_image_layout(
                command_buffer,
                info.image.get_handle(),
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
                    .layerCount = layer_count
                },
                .imageExtent = {
                    static_cast<uint32_t>(spec.width),
                    static_cast<uint32_t>(spec.height),
                    static_cast<uint32_t>(spec.depth)
                }
            };

            // Copy mip levels from staging buffer
            command_buffer.copyBufferToImage(
                staging_buffer.get_handle(),
                info.image.get_handle(),
                vk::ImageLayout::eTransferDstOptimal,
                {copy_region}
                );

            const size_t mip_count = spec.generate_mipmaps ? get_mip_level_count() : 1;
            if (mip_count > 1)
            {
                // There are mips to generate, move to get ready to transfer
                portal::renderer::vulkan::transition_image_layout(
                    command_buffer,
                    info.image.get_handle(),
                    range,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eTransferSrcOptimal,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::AccessFlagBits2::eTransferRead,
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::PipelineStageFlagBits2::eTransfer
                    );
            }
            else
            {
                // There are mips to generate, move to get ready to transfer
                portal::renderer::vulkan::transition_image_layout(
                    command_buffer,
                    info.image.get_handle(),
                    range,
                    vk::ImageLayout::eTransferDstOptimal,
                    image->get_descriptor_image_info().imageLayout,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::AccessFlagBits2::eShaderRead,
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::PipelineStageFlagBits2::eFragmentShader
                    );
            }
        }
        );

    const size_t mip_count = spec.generate_mipmaps ? get_mip_level_count() : 1;
    if (mip_count > 1 && spec.generate_mipmaps)
        generate_mipmaps();
}

void VulkanTexture::generate_mipmaps() const
{
    auto& info = image->get_image_info();

    device.immediate_submit(
        [&](const vk::raii::CommandBuffer& command_buffer)
        {
            const auto mip_levels = get_mip_level_count();
            const auto layer_count = get_array_layer_count();

            for (uint32_t face = 0; face < layer_count; ++face)
            {
                for (uint32_t i = 1; i < mip_levels; ++i)
                {
                    vk::ImageBlit2 blit{
                        .srcSubresource = {
                            .aspectMask = vk::ImageAspectFlagBits::eColor,
                            .mipLevel = i - 1,
                            .baseArrayLayer = face,
                            .layerCount = 1
                        },
                        .srcOffsets = std::array{vk::Offset3D{},
                                                 vk::Offset3D{static_cast<int32_t>(spec.width) >> (i - 1),
                                                              static_cast<int32_t>(spec.height) >> (i - 1),
                                                              static_cast<int32_t>(spec.depth) >> (i - 1)}},

                        .dstSubresource = {
                            .aspectMask = vk::ImageAspectFlagBits::eColor,
                            .mipLevel = i,
                            .baseArrayLayer = face,
                            .layerCount = 1
                        },
                        .dstOffsets = std::array{vk::Offset3D{},
                                                 vk::Offset3D{static_cast<int32_t>(spec.width) >> i,
                                                              static_cast<int32_t>(spec.height) >> i,
                                                              static_cast<int32_t>(spec.depth) >> i}},
                    };

                    vk::ImageSubresourceRange range{
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel = i,
                        .levelCount = 1,
                        .baseArrayLayer = face,
                        .layerCount = 1
                    };

                    portal::renderer::vulkan::transition_image_layout(
                        command_buffer,
                        info.image.get_handle(),
                        range,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::AccessFlagBits2::eNone,
                        vk::AccessFlagBits2::eTransferWrite,
                        vk::PipelineStageFlagBits2::eTransfer,
                        vk::PipelineStageFlagBits2::eTransfer
                        );

                    vk::BlitImageInfo2 blit_info{
                        .srcImage = info.image.get_handle(),
                        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
                        .dstImage = info.image.get_handle(),
                        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                        .regionCount = 1,
                        .pRegions = &blit,
                        .filter = utils::to_filter(spec.sampler_spec.value_or(SamplerSpecification{}).filter),
                    };
                    command_buffer.blitImage2(blit_info);

                    portal::renderer::vulkan::transition_image_layout(
                        command_buffer,
                        info.image.get_handle(),
                        range,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eTransferSrcOptimal,
                        vk::AccessFlagBits2::eTransferWrite,
                        vk::AccessFlagBits2::eTransferRead,
                        vk::PipelineStageFlagBits2::eTransfer,
                        vk::PipelineStageFlagBits2::eTransfer
                        );
                }
            }

            // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
            const vk::ImageSubresourceRange range{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = mip_levels,
                .baseArrayLayer = 0,
                .layerCount = layer_count
            };

            portal::renderer::vulkan::transition_image_layout(
                command_buffer,
                info.image.get_handle(),
                range,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::AccessFlagBits2::eTransferRead,
                vk::AccessFlagBits2::eShaderRead,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::PipelineStageFlagBits2::eFragmentShader
                );
        }
        );
}

uint32_t VulkanTexture::get_array_layer_count() const
{
    if (spec.type == TextureType::TextureCube)
        return 6;
    return 1;
}

} // portal
