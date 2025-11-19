//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/reference.h"
#include "portal/engine/renderer/image/image_types.h"

namespace portal::renderer
{

enum class AttachmentLoadOperator
{
    Inherit,
    Clear,
    Load
};

enum class BlendMode
{
    None,
    OneZero,
    SrcAlphaOneMinusSrcAlpha,
    Additive,
    ZeroSrcColor
};

struct RenderTargetTextureProperties
{
    ImageFormat format{};
    bool blend = true;

    BlendMode blend_mode = BlendMode::SrcAlphaOneMinusSrcAlpha;
    AttachmentLoadOperator load_operator = AttachmentLoadOperator::Inherit;
};

struct RenderTargetAttachmentProperties
{
    std::vector<RenderTargetTextureProperties> color_attachments;

    // Master switch (individual attachments can be disabled in RenderTargetTextureProperties)
    bool blend = true;
    // None means use BlendMode in RenderTargetTextureProperties
    BlendMode blend_mode = BlendMode::None;
};

struct RenderTargetProperties
{
    float scale = 1.f;
    size_t width = 0;
    size_t height = 0;
    glm::vec4 clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
    float depth_clear_value = 0.f;
    bool clear_color_on_load = true;
    bool clear_depth_on_load = true;

    RenderTargetAttachmentProperties attachments;
    uint32_t samples = 1; // multisampling

    // Will it be used for transfer ops?
    bool transfer = false;
    StringId name;
};

class RenderTarget
{
public:
    virtual ~RenderTarget() = default;

    virtual void resize(size_t width, size_t height, bool force_recreate) = 0;

    [[nodiscard]] virtual size_t get_width() const = 0;
    [[nodiscard]] virtual size_t get_height() const = 0;

    [[nodiscard]] virtual Reference<Image> get_image(size_t index) const = 0;
    [[nodiscard]] virtual size_t get_color_attachment_count() const = 0;

    [[nodiscard]] virtual bool has_depth_attachment() const = 0;
    [[nodiscard]] virtual Reference<Image> get_depth_image() const = 0;

    [[nodiscard]] virtual const RenderTargetProperties& get_properties() const = 0;
};
} // portal