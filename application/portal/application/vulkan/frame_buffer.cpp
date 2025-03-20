//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "frame_buffer.h"

#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/image_view.h"
#include "portal/application/vulkan/render_pass.h"
#include "portal/application/vulkan/render_target.h"

namespace portal::vulkan
{
Framebuffer::Framebuffer(Device& device, const RenderTarget& render_target, const RenderPass& render_pass): VulkanResource(nullptr, &device)
{
    std::vector<vk::ImageView> attachments;
    for (auto& view : render_target.get_views())
    {
        attachments.emplace_back(view.get_handle());
    }

    const vk::FramebufferCreateInfo create_info(
        {},
        render_pass.get_handle(),
        attachments,
        render_target.get_extent().width,
        render_target.get_extent().height,
        1
    );
    set_handle(device.get_handle().createFramebuffer(create_info));
    if (!get_handle())
        throw std::runtime_error("Failed to create framebuffer");
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept: VulkanResource(std::move(other)), extent(std::exchange(other.extent, {})) {}

Framebuffer::~Framebuffer()
{
    if (get_handle())
        get_device().get_handle().destroyFramebuffer(get_handle());
}

vk::Extent2D Framebuffer::get_extent() const
{
    return extent;
}
} // portal
