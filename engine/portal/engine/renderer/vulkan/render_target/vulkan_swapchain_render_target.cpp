//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_swapchain_render_target.h"

#include "portal/engine/renderer/vulkan/image/vulkan_image.h"

namespace portal::renderer::vulkan
{
VulkanSwapchainRenderTarget::VulkanSwapchainRenderTarget(const RenderTargetProperties& prop, VulkanSwapchain& swapchain)
    : VulkanRenderTarget(prop),
      swapchain(swapchain)
{
    make_depth_image();
}


void VulkanSwapchainRenderTarget::begin_frame(FrameContext& frame)
{
    auto& swapchain_image_data = swapchain.begin_frame(frame);

    const auto rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);
    const auto draw_image = make_reference<VulkanImage>(swapchain_image_data.image, swapchain_image_data.image_properties, swapchain.get_context());

    // TODO: Make it a sub-frame context? (as we can have multiple render targets in a single frame?)
    rendering_context->image_context = FrameDrawImageContext{
        .draw_image = draw_image,
        .draw_image_view = make_reference<VulkanImageView>(*swapchain_image_data.image_view, ImageViewProperties{.image = draw_image.get(), .mip = 0}, swapchain.get_context()),
        .depth_image = depth_image,
        .depth_image_view = depth_image->get_view(),
    };
}

void VulkanSwapchainRenderTarget::end_frame(const FrameContext& frame)
{
    swapchain.present(frame);
}

void VulkanSwapchainRenderTarget::resize(const size_t new_width, const size_t new_height, const bool force_recreate)
{
    VulkanRenderTarget::resize(new_width, new_height, force_recreate);
    depth_image->resize(new_width, new_height);
}

void VulkanSwapchainRenderTarget::inner_initialize()
{
    make_depth_image();
}

void VulkanSwapchainRenderTarget::inner_release()
{
    depth_image.reset();
}

void VulkanSwapchainRenderTarget::make_depth_image()
{
    image::Properties image_properties{
        .format = get_depth_format(),
        .usage = ImageUsage::Attachment,
        .transfer = true,
        .width = static_cast<size_t>(prop.width * prop.scale),
        .height = static_cast<size_t>(prop.height * prop.scale)
    };
    depth_image = make_reference<VulkanImage>(image_properties, swapchain.get_context());
}
} // portal
