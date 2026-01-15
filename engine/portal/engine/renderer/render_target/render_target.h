//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file render_target.h
 * @brief Render target abstraction
 */

#pragma once

#include <glaze/reflection/get_name.hpp>

#include "portal/application/frame_context.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/rendering_context.h"
#include "portal/engine/renderer/image/image.h"
#include "portal/engine/renderer/image/image_types.h"

namespace portal::renderer
{
/**
 * @enum BlendMode
 * @brief Attachment blending modes
 *
 * Defines color blending for render target attachments.
 */
enum class BlendMode
{
    None,                           ///< No blending
    OneZero,                        ///< Replace (src, no blending)
    SrcAlphaOneMinusSrcAlpha,      ///< Standard alpha blending
    Additive,                       ///< Additive blending
    ZeroSrcColor                    ///< Multiply (dest * src)
};

/**
 * @enum AttachmentLoadOperator
 * @brief Attachment load operation
 *
 * Controls how attachment contents are initialized at render pass start.
 */
enum class AttachmentLoadOperator
{
    Inherit,    ///< Preserve existing contents
    Clear,      ///< Clear to specified value
    Load        ///< Load previous contents
};

/**
 * @struct AttachmentTextureProperty
 * @brief Render target attachment configuration
 *
 * Defines format, blending, and load behavior for a single attachment.
 */
struct AttachmentTextureProperty
{
    ImageFormat format{};
    bool blend = true;
    BlendMode blend_mode = BlendMode::SrcAlphaOneMinusSrcAlpha;
    AttachmentLoadOperator load_operator = AttachmentLoadOperator::Inherit;
};

/**
 * @struct AttachmentProperties
 * @brief Render target attachment list
 *
 * Depth attachment is always last in attachment_images.
 */
struct AttachmentProperties
{
    std::vector<AttachmentTextureProperty> attachment_images;

    // TODO: move depth attachment out of the vector as an optional member?

    bool blend = true;                      ///< Master blend switch
    BlendMode blend_mode = BlendMode::None; ///< None uses per-attachment blend mode
};

/**
 * @struct RenderTargetProperties
 * @brief Render target configuration
 *
 * Defines dimensions, attachments, clear values, multisampling, and transfer support.
 */
struct RenderTargetProperties
{
    float scale = 1.f;
    size_t width = 0;
    size_t height = 0;
    glm::vec4 clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
    float depth_clear_value = 0.f;
    bool clear_color_on_load = true;
    bool clear_depth_on_load = true;

    AttachmentProperties attachments;
    uint32_t samples = 1; ///< MSAA sample count

    bool transfer = false; ///< Enable transfer operations

    // A list of existing images to attach the render target to (from the swapchain)
    std::unordered_map<size_t, Reference<Image>> existing_images;

    StringId name;
};

/**
 * @class RenderTarget
 * @brief Abstract render target interface
 *
 * Framebuffer with color and optional depth attachments.
 */
class RenderTarget
{
public:
    virtual ~RenderTarget() = default;

    /** @brief Resizes render target */
    virtual void resize(size_t width, size_t height, bool force_recreate) = 0;

    /** @brief Gets render target width */
    [[nodiscard]] virtual size_t get_width() const = 0;

    /** @brief Gets render target height */
    [[nodiscard]] virtual size_t get_height() const = 0;

    /** @brief Gets color attachment count */
    [[nodiscard]] virtual size_t get_color_attachment_count() const = 0;

    [[nodiscard]] virtual std::span<const ImageFormat> get_color_formats() const = 0;

    /** @brief Checks if render target has depth attachment */
    [[nodiscard]] virtual bool has_depth_attachment() const = 0;

    /** @brief Gets render target properties */
    [[nodiscard]] virtual const RenderTargetProperties& get_properties() const = 0;

    [[nodiscard]] virtual size_t get_color_images_count() const = 0;
    [[nodiscard]] virtual Reference<Image> get_image(size_t attachment_index) = 0;

    [[nodiscard]] virtual bool has_depth_image() const = 0;
    [[nodiscard]] virtual Reference<Image> get_depth_image() const = 0;
};
} // portal
