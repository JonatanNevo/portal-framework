//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file vulkan_render_target.h
 * @brief Vulkan render target implementation
 */

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

/**
 * @class VulkanRenderTarget
 * @brief Vulkan render target
 *
 * Creates vk::RenderingInfo with color and depth attachments, clear values, and blending.
 * Supports dynamic rendering (no explicit framebuffer objects).
 */
class VulkanRenderTarget final : public RenderTarget
{
public:
    /**
     * @brief Constructs Vulkan render target
     * @param prop Render target configuration
     */
    VulkanRenderTarget(const RenderTargetProperties& prop);
    ~VulkanRenderTarget() override;

    /** @brief Resizes render target, recreating attachments */
    void resize(size_t new_width, size_t new_height, bool force_recreate) override;

    /**
     * @brief Creates vk::RenderingInfo from frame context
     * @param frame_context Current frame rendering context
     * @return Vulkan rendering info structure
     */
    vk::RenderingInfo make_rendering_info(const FrameRenderingContext& frame_context);

    /**
     * @brief Creates vk::RenderingInfo from custom attachments
     * @param color_images Color attachment views
     * @param depth_image Optional depth attachment view
     * @return Vulkan rendering info structure
     */
    vk::RenderingInfo make_rendering_info(std::span<vk::ImageView> color_images, const std::optional<vk::ImageView>& depth_image);

    /** @brief Gets render target width */
    [[nodiscard]] size_t get_width() const override;

    /** @brief Gets render target height */
    [[nodiscard]] size_t get_height() const override;

    /** @brief Gets color attachment count */
    [[nodiscard]] size_t get_color_attachment_count() const override;

    /** @brief Checks if render target has depth attachment */
    [[nodiscard]] bool has_depth_attachment() const override;

    /** @brief Gets render target properties */
    [[nodiscard]] const RenderTargetProperties& get_properties() const override;

    /** @brief Gets depth attachment format */
    [[nodiscard]] ImageFormat get_depth_format() const;

    /** @brief Gets color attachment formats */
    [[nodiscard]] std::span<const ImageFormat> get_color_formats() const;

protected:
    /** @brief Creates Vulkan attachments */
    void initialize();

    /** @brief Releases Vulkan resources */
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
