//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vector>
#include "portal/core/glm.h"

#include "portal/engine/renderer/image/image_types.h"

namespace portal::renderer
{

enum class AttachmentLoadOperator
{
    Inherit,
    Clear,
    Load
};

namespace render_target
{
    enum class BlendMode
    {
        None,
        OneZero,
        SrcAlphaOneMinusSrcAlpha,
        Additive,
        ZeroSrcColor
    };

    struct TextureSpecification
    {
        ImageFormat format;
        bool blend = true;
        BlendMode blend_mode = BlendMode::SrcAlphaOneMinusSrcAlpha;
        AttachmentLoadOperator load_operator = AttachmentLoadOperator::Inherit;
    };

    struct AttachmentSpecification
    {
        std::vector<TextureSpecification> attachments;
    };

    struct Specification
    {
        float scale = 1.f;
        size_t width = 0;
        size_t height = 0;
        glm::vec4 clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
        float depth_clear_value = 0.f;
        bool clear_color_on_load = true;
        bool clear_depth_on_load = true;

        AttachmentSpecification attachments;
        uint32_t samples = 1; // multisampling

        // Master switch (individual attachments can be disabled in FrameBufferTextureSpecification)
        bool blend = true;
        // None means use BlendMode in FrameBufferTextureSpecification
        BlendMode blend_mode = BlendMode::None;

        // Will it be used for transfer ops?
        bool transfer = false;
        StringId name;
    };
}
}
