//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once
#include "base/vulkan_resource.h"

namespace portal::vulkan
{
class Device;
class RenderPass;
class RenderTarget;

class Framebuffer final : public VulkanResource<vk::Framebuffer>
{
public:
    Framebuffer(Device& device, const RenderTarget& render_target, const RenderPass& render_pass);
    Framebuffer(Framebuffer&& other) noexcept;
    ~Framebuffer() override;

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer& operator=(Framebuffer&& other) = delete;

    vk::Extent2D get_extent() const;
private:
    vk::Extent2D extent;
};

} // portal
