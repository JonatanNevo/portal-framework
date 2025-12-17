//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/reference.h"
#include "portal/engine/renderer/render_target/render_target.h"

namespace portal::renderer
{
struct FrameRenderingContext;
}

namespace portal::renderer::vulkan
{
class VulkanContext;
class VulkanImage;

class VulkanRenderTarget final : public RenderTarget
{
public:
    VulkanRenderTarget(const RenderTargetProperties& prop);
    ~VulkanRenderTarget() override;

    void resize(size_t new_width, size_t new_height, bool force_recreate) override;

    vk::RenderingInfo make_rendering_info(const FrameRenderingContext& frame_context);
    vk::RenderingInfo make_rendering_info(std::span<vk::ImageView> color_images, const std::optional<vk::ImageView>& depth_image);

    [[nodiscard]] size_t get_width() const override;
    [[nodiscard]] size_t get_height() const override;

    [[nodiscard]] size_t get_color_attachment_count() const override;
    [[nodiscard]] bool has_depth_attachment() const override;

    [[nodiscard]] const RenderTargetProperties& get_properties() const override;

    [[nodiscard]] ImageFormat get_depth_format() const;
    [[nodiscard]] std::span<const ImageFormat> get_color_formats() const;

protected:
    void initialize();
    void release();

private:
    RenderTargetProperties prop;
    size_t width = 0, height = 0;

    std::vector<ImageFormat> color_formats;
    std::optional<ImageFormat> depth_format;

    std::vector<vk::RenderingAttachmentInfo> rendering_attachments;
    vk::RenderingAttachmentInfo depth_rendering;
    vk::RenderingInfo rendering_info;
};
}
