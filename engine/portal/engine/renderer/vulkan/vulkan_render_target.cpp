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

vk::AttachmentLoadOp to_load_op(const render_target::Specification& spec, const render_target::TextureSpecification& attachment)
{
    if (attachment.load_operator == AttachmentLoadOperator::Inherit)
    {
        if (utils::is_depth_format(attachment.format))
            return spec.clear_depth_on_load ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;

        return spec.clear_color_on_load ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    }

    return attachment.load_operator == AttachmentLoadOperator::Clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
}

VulkanRenderTarget::VulkanRenderTarget(
    const render_target::Specification& specs,
    const Ref<VulkanContext>& context
    ) : context(context), spec(specs)
{
    width = specs.width;
    height = specs.height;

    size_t index = 0;
    color_attachments.reserve(specs.attachments.attachments.size());
    for (auto& attachment_spec : specs.attachments.attachments)
    {
        image::Specification image_spec{
            .format = attachment_spec.format,
            .usage = ImageUsage::Attachment,
            .transfer = specs.transfer,
            .width = static_cast<size_t>(width * specs.scale),
            .height = static_cast<size_t>(height * specs.scale)
        };

        if (utils::is_depth_format(attachment_spec.format))
        {
            image_spec.name = STRING_ID(fmt::format("{}_depth_attachment_{}", specs.name.string, index));
            depth_attachment = Ref<VulkanImage>::create(image_spec, context);
        }
        else
        {
            image_spec.name = STRING_ID(fmt::format("{}_color_attachment_{}", specs.name.string, index));
            color_attachments.emplace_back(Ref<VulkanImage>::create(image_spec, context));
        }

        index++;
    }

    resize(width, height, true);
}

void VulkanRenderTarget::initialize()
{
    release();

    // Using vulkan 1.4, no need to create frame buffer object
    rendering_attachments.reserve(spec.attachments.attachments.size());

    size_t index = 0;
    for (auto& attachment_spec : spec.attachments.attachments)
    {
        if (utils::is_depth_format(attachment_spec.format))
        {
            auto& image_spec = depth_attachment->get_specs();
            image_spec.width = static_cast<size_t>(width * spec.scale);
            image_spec.height = static_cast<size_t>(height * spec.scale);
            depth_attachment->initialize();

            depth_rendering = vk::RenderingAttachmentInfo{
                .imageView = depth_attachment->get_image_info().view,
                .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
                .loadOp = to_load_op(spec, attachment_spec),
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = vk::ClearDepthStencilValue{spec.depth_clear_value, 0}
            };
        }
        else
        {
            auto& image = color_attachments[index];
            auto& image_spec = image->get_specs();
            image_spec.width = static_cast<size_t>(width * spec.scale);
            image_spec.height = static_cast<size_t>(height * spec.scale);
            image->initialize();

            auto& rendering_attachment = rendering_attachments.emplace_back();
            rendering_attachment = vk::RenderingAttachmentInfo{
                .imageView = image->get_image_info().view,
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp = to_load_op(spec, attachment_spec),
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = vk::ClearColorValue{spec.clear_color.r, spec.clear_color.g, spec.clear_color.b, spec.clear_color.a}
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
    size_t index = 0;
    for (auto& image : color_attachments)
    {
        image->release();
        index++;
    }

    if (depth_attachment)
        depth_attachment->release();

    rendering_attachments.clear();
}

void VulkanRenderTarget::resize(const size_t new_width, const size_t new_height, const bool force_recreate)
{
    if (!force_recreate && width == new_width && height == new_height)
        return;

    width = static_cast<size_t>(new_width * spec.scale);
    height = static_cast<size_t>(new_height * spec.scale);;
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

Ref<Image> VulkanRenderTarget::get_image(const size_t index) const
{
    PORTAL_ASSERT(index < color_attachments.size(), "Invalid color attachment index");
    return color_attachments[index];
}

bool VulkanRenderTarget::has_depth_attachment() const
{
    return depth_attachment != nullptr;
}

Ref<Image> VulkanRenderTarget::get_depth_image() const
{
    return depth_attachment;
}

const render_target::Specification& VulkanRenderTarget::get_spec() const
{
    return spec;
}

const vk::RenderingInfo& VulkanRenderTarget::get_rendering_info() const
{
    return rendering_info;
}
}
