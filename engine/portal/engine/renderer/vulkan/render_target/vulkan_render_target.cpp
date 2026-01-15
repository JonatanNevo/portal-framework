//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_render_target.h"

#include "llvm/ADT/SmallVector.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/application/frame_context.h"
#include "portal/engine/renderer/rendering_context.h"

namespace portal::renderer::vulkan
{
vk::AttachmentLoadOp to_load_op(const RenderTargetProperties& prop, const AttachmentTextureProperty& attachment)
{
    if (attachment.load_operator == AttachmentLoadOperator::Inherit)
    {
        if (utils::is_depth_format(attachment.format))
            return prop.clear_depth_on_load ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;

        return prop.clear_color_on_load ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    }

    return attachment.load_operator == AttachmentLoadOperator::Clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
}

VulkanRenderTarget::VulkanRenderTarget(
    const RenderTargetProperties& prop,
    const VulkanContext& context
) : prop(prop), context(context)
{
    width = static_cast<size_t>(prop.width * prop.scale);
    height = static_cast<size_t>(prop.height * prop.scale);

    color_formats.reserve(prop.attachments.attachment_images.size());
    size_t attachment_index = 0;
    for (const auto& attachment : prop.attachments.attachment_images)
    {
        if (utils::is_depth_format(attachment.format))
        {
            PORTAL_ASSERT(!depth_format.has_value(), "Multiple depth images requested");
            depth_format = attachment.format;
            if (prop.existing_images.contains(attachment_index))
            {
                depth_image = reference_cast<VulkanImage>(prop.existing_images.at(attachment_index));
                PORTAL_ASSERT(depth_image != nullptr, "Invalid depth image reference");
            }
            else
            {
                image::Properties depth_image_properties{
                    .format = attachment.format,
                    .usage = ImageUsage::SubAttachment,
                    .transfer = prop.transfer,
                    .width = width,
                    .height = height,
                    .name = STRING_ID(
                        fmt::format("{}_depth_image_{}", prop.name != INVALID_STRING_ID ? prop.name.string : "Unnamed render target", attachment_index
                        )
                    )
                };
                depth_image = make_reference<VulkanImage>(depth_image_properties, context);
            }
        }
        else
        {
            color_formats.push_back(attachment.format);
            if (prop.existing_images.contains(attachment_index))
            {
                // Will be filled out later
                color_images.emplace_back();
            }
            else
            {
                image::Properties depth_image_properties{
                    .format = attachment.format,
                    .usage = ImageUsage::SubAttachment,
                    .transfer = prop.transfer,
                    .width = width,
                    .height = height,
                    .name = STRING_ID(
                        fmt::format("{}_color_image_{}", prop.name != INVALID_STRING_ID ? prop.name.string : "Unnamed render target", attachment_index
                        )
                    )
                };
                color_images.emplace_back(make_reference<VulkanImage>(depth_image_properties, context));
            }
        }

        attachment_index++;
    }

    initialize();
}

VulkanRenderTarget::~VulkanRenderTarget()
{
    release();
}

void VulkanRenderTarget::initialize()
{
    release();

    // Using vulkan 1.4, no need to create frame buffer object
    rendering_attachments.reserve(prop.attachments.attachment_images.size());

    size_t attachment_index = 0;
    for (auto& attachment_prop : prop.attachments.attachment_images)
    {
        if (utils::is_depth_format(attachment_prop.format))
        {
            depth_rendering = vk::RenderingAttachmentInfo{
                .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
                .loadOp = to_load_op(prop, attachment_prop),
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = vk::ClearDepthStencilValue{prop.depth_clear_value, 0}
            };

            if (prop.existing_images.contains(attachment_index))
            {
                depth_image = reference_cast<VulkanImage>(prop.existing_images.at(attachment_index));
                PORTAL_ASSERT(utils::is_depth_format(depth_image->get_format()), "Trying to attach non-depth image as depth attachment");
            }
            else
            {
                depth_image->resize(width, height);
            }
        }
        else
        {
            if (prop.existing_images.contains(attachment_index))
            {
                const auto color_image = reference_cast<VulkanImage>(prop.existing_images.at(attachment_index));
                PORTAL_ASSERT(!utils::is_depth_format(color_image->get_format()), "Trying to attach depth image as color attachment");
                color_images[attachment_index] = color_image;
            }
            else
            {
                color_images[attachment_index]->resize(width, height);
                color_images[attachment_index]->create_per_layer_image_view();
            }

            rendering_attachments.emplace_back(
                vk::RenderingAttachmentInfo{
                    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                    .loadOp = to_load_op(prop, attachment_prop),
                    .storeOp = vk::AttachmentStoreOp::eStore,
                    .clearValue = vk::ClearColorValue{prop.clear_color.r, prop.clear_color.g, prop.clear_color.b, prop.clear_color.a}
                }
            );
        }

        attachment_index++;
    }

    rendering_info = {
        .renderArea = {{}, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}},
        .layerCount = 1,
        .colorAttachmentCount = static_cast<uint32_t>(rendering_attachments.size()),
        .pColorAttachments = rendering_attachments.data(),
        .pDepthAttachment = &depth_rendering,
        .pStencilAttachment = nullptr
    };
}

void VulkanRenderTarget::release()
{
    rendering_attachments.clear();

    size_t attachment_index = 0;
    for (const auto& image : color_images)
    {
        if (prop.existing_images.contains(attachment_index))
            continue;

        if (image->get_prop().layers == 1 || attachment_index == 0 && !image->get_layer_image_view(0))
            image->release();
        attachment_index++;
    }

    if (depth_image)
    {
        // Release the depth image only of we are owning it
        if (!prop.existing_images.contains(prop.attachments.attachment_images.size() - 1))
            depth_image->release();
    }
}

bool VulkanRenderTarget::resize(const size_t new_width, const size_t new_height, const bool force_recreate)
{
    if (!force_recreate && width == new_width && height == new_height)
        return false;

    width = static_cast<size_t>(new_width * prop.scale);
    height = static_cast<size_t>(new_height * prop.scale);
    depth_image->resize(new_width, new_height);
    initialize();
    return true;
}

vk::RenderingInfo VulkanRenderTarget::make_rendering_info()
{
    if (depth_image)
    {
        depth_rendering.imageView = reference_cast<VulkanImageView>(depth_image->get_view())->get_vk_image_view();
    }

    size_t index = 0;
    for (const auto& image : color_images)
    {
        rendering_attachments[index++].imageView = reference_cast<VulkanImageView>(image->get_view())->get_vk_image_view();
    }

    return rendering_info;
}


size_t VulkanRenderTarget::get_width() const
{
    return width;
}

size_t VulkanRenderTarget::get_height() const
{
    return height;
}

glm::uvec4 VulkanRenderTarget::get_viewport_bounds() const
{
    return {
        0,
        0,
        static_cast<uint32_t>(get_width()),
        static_cast<uint32_t>(get_height())
    };
}

size_t VulkanRenderTarget::get_color_attachment_count() const
{
    return color_formats.size();
}


bool VulkanRenderTarget::has_depth_attachment() const
{
    return depth_format.has_value();
}

const RenderTargetProperties& VulkanRenderTarget::get_properties() const
{
    return prop;
}

ImageFormat VulkanRenderTarget::get_depth_format() const
{
    return depth_format.value_or(ImageFormat::None);
}

std::span<const ImageFormat> VulkanRenderTarget::get_color_formats() const
{
    return std::span{color_formats};
}

size_t VulkanRenderTarget::get_color_images_count() const
{
    return color_images.size();
}

Reference<Image> VulkanRenderTarget::get_image(const size_t attachment_index)
{
    return color_images.at(attachment_index);
}

bool VulkanRenderTarget::has_depth_image() const
{
    return depth_image != nullptr;
}

Reference<Image> VulkanRenderTarget::get_depth_image() const
{
    return depth_image;
}
}
