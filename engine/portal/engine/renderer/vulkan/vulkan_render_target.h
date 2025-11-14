//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/reference.h"
#include "portal/engine/renderer/render_target/render_target.h"

namespace portal::renderer::vulkan
{
class VulkanContext;
class VulkanImage;

class VulkanRenderTarget final: public RenderTarget
{
public:
    VulkanRenderTarget(const render_target::Specification& specs, const VulkanContext& context);
    ~VulkanRenderTarget() override;

    void resize(size_t new_width, size_t new_height, bool force_recreate) override;
    void initialize();
    void release();

    [[nodiscard]] size_t get_width() const override;
    [[nodiscard]] size_t get_height() const override;

    [[nodiscard]] size_t get_color_attachment_count() const override;
    [[nodiscard]] Reference<Image> get_image(size_t index) const override;

    [[nodiscard]] bool has_depth_attachment() const override;
    [[nodiscard]] Reference<Image> get_depth_image() const override;

    [[nodiscard]] const render_target::Specification& get_spec() const override;

    [[nodiscard]] const vk::RenderingInfo& get_rendering_info() const;


private:
    render_target::Specification spec;
    size_t width = 0, height = 0;

    std::vector<Reference<VulkanImage>> color_attachments;
    Reference<VulkanImage> depth_attachment;

    std::vector<vk::RenderingAttachmentInfo> rendering_attachments;
    vk::RenderingAttachmentInfo depth_rendering;
    vk::RenderingInfo rendering_info;
};
}
