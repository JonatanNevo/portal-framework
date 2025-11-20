//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_render_target.h"

#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "../draw_context.h"

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
    const RenderTargetProperties& prop
) : prop(prop)
{
    width = prop.width;
    height = prop.height;

    color_formats.reserve(prop.attachments.attachment_images.size());
    for (const auto& attachment : prop.attachments.attachment_images)
    {
        if (utils::is_depth_format(attachment.format))
        {
            PORTAL_ASSERT(!depth_format.has_value(), "Multiple depth images requested");
            depth_format = attachment.format;
        }
        else
        {
            color_formats.push_back(attachment.format);
        }
    }

    resize(width, height, true);
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

    size_t index = 0;
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
        }
        else
        {
            rendering_attachments.emplace_back(
                vk::RenderingAttachmentInfo{
                    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                    .loadOp = to_load_op(prop, attachment_prop),
                    .storeOp = vk::AttachmentStoreOp::eStore,
                    .clearValue = vk::ClearColorValue{prop.clear_color.r, prop.clear_color.g, prop.clear_color.b, prop.clear_color.a}
                }
            );
        }
        index++;
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
}

void VulkanRenderTarget::resize(const size_t new_width, const size_t new_height, const bool force_recreate)
{
    if (!force_recreate && width == new_width && height == new_height)
        return;

    width = static_cast<size_t>(new_width * prop.scale);
    height = static_cast<size_t>(new_height * prop.scale);;
    initialize();
}

vk::RenderingInfo VulkanRenderTarget::make_rendering_info(const DrawContext& draw_context)
{
    llvm::SmallVector<vk::ImageView, 4> color_attachments = {draw_context.draw_image_view};
    const std::optional depth_attachment = draw_context.depth_image_view;

    return make_rendering_info(color_attachments, depth_attachment);
}

vk::RenderingInfo VulkanRenderTarget::make_rendering_info(
    const std::span<vk::ImageView> color_images,
    const std::optional<vk::ImageView>& depth_image
)
{
    if (depth_image.has_value())
    {
        PORTAL_ASSERT(depth_format.has_value(), "Depth image requested but no depth attachment");
        depth_rendering.imageView = depth_image.value();
    }

    PORTAL_ASSERT(color_images.size() == rendering_attachments.size(), "Invalid number of color attachments");

    size_t index = 0;
    for (const auto& image : color_images)
    {
        rendering_attachments[index++].imageView = image;
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
}
