//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_render_target.h"

#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/engine/scene/draw_context.h"

namespace portal::renderer::vulkan
{

vk::AttachmentLoadOp to_load_op(const RenderTargetProperties& prop, const RenderTargetTextureProperties& attachment)
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
    ) : prop(prop)
{
    width = prop.width;
    height = prop.height;

    size_t index = 0;
    color_attachments.reserve(prop.attachments.color_attachments.size());
    for (auto& attachment_spec : prop.attachments.color_attachments)
    {
        image::Specification image_spec{
            .format = attachment_spec.format,
            .usage = ImageUsage::Attachment,
            .transfer = prop.transfer,
            .width = static_cast<size_t>(width * prop.scale),
            .height = static_cast<size_t>(height * prop.scale)
        };

        if (utils::is_depth_format(attachment_spec.format))
        {
            image_spec.name = STRING_ID(std::format("{}_depth_attachment_{}", prop.name.string, index));
            depth_attachment = make_reference<VulkanImage>(image_spec, context);
        }
        else
        {
            image_spec.name = STRING_ID(std::format("{}_color_attachment_{}", prop.name.string, index));
            color_attachments.emplace_back(make_reference<VulkanImage>(image_spec, context));
        }

        index++;
    }

    resize(width, height, true);
}

VulkanRenderTarget::~VulkanRenderTarget()
{
    release();
    color_attachments.clear();
    depth_attachment.reset();
}

void VulkanRenderTarget::initialize()
{
    release();

    // Using vulkan 1.4, no need to create frame buffer object
    rendering_attachments.reserve(prop.attachments.color_attachments.size());

    size_t index = 0;
    for (auto& attachment_spec : prop.attachments.color_attachments)
    {
        if (utils::is_depth_format(attachment_spec.format))
        {
            depth_attachment->resize(static_cast<size_t>(width * prop.scale), static_cast<size_t>(height * prop.scale));

            depth_rendering = vk::RenderingAttachmentInfo{
                .imageView = depth_attachment->get_image_info().view,
                .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
                .loadOp = to_load_op(prop, attachment_spec),
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = vk::ClearDepthStencilValue{prop.depth_clear_value, 0}
            };
        }
        else
        {
            const auto& image = color_attachments[index];
            image->resize(static_cast<size_t>(width * prop.scale), static_cast<size_t>(height * prop.scale));

            auto& rendering_attachment = rendering_attachments.emplace_back();
            rendering_attachment = vk::RenderingAttachmentInfo{
                .imageView = image->get_image_info().view,
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp = to_load_op(prop, attachment_spec),
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = vk::ClearColorValue{prop.clear_color.r, prop.clear_color.g, prop.clear_color.b, prop.clear_color.a}
            };
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
    for (const auto& image : color_attachments)
    {
        image->release();
    }

    if (depth_attachment)
        depth_attachment->release();

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
    return color_attachments.size();
}

Reference<Image> VulkanRenderTarget::get_image(const size_t index) const
{
    PORTAL_ASSERT(index < color_attachments.size(), "Invalid color attachment index");
    return color_attachments[index];
}

bool VulkanRenderTarget::has_depth_attachment() const
{
    return depth_attachment != nullptr;
}

Reference<Image> VulkanRenderTarget::get_depth_image() const
{
    return depth_attachment;
}

const RenderTargetProperties& VulkanRenderTarget::get_properties() const
{
    return prop;
}

const vk::RenderingInfo& VulkanRenderTarget::get_rendering_info() const
{
    return rendering_info;
}
}
