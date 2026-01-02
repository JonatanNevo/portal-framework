//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/buffer.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/renderer_resource.h"
#include "portal/engine/renderer/image/image_types.h"

namespace portal::renderer
{
/**
 * @class Texture
 * @brief Abstract texture interface with mipmap, sampler, and cube texture support
 *
 * Wraps Image with texture-specific functionality: mipmap queries, sampler configuration,
 * predefined texture IDs (missing/white/black), and texture type (2D/cube).
 * Supports CPU buffer access and resizing.
 */
class Texture : public RendererResource
{
public:
    const static StringId MISSING_TEXTURE_ID;
    const static StringId WHITE_TEXTURE_ID;
    const static StringId BLACK_TEXTURE_ID;

public:
    explicit Texture(const StringId& id) : RendererResource(id) {};

    /** @brief Gets texture format */
    [[nodiscard]] virtual ImageFormat get_format() const = 0;

    /** @brief Gets texture width */
    [[nodiscard]] virtual size_t get_width() const = 0;

    /** @brief Gets texture height */
    [[nodiscard]] virtual size_t get_height() const = 0;

    /** @brief Gets texture depth */
    [[nodiscard]] virtual size_t get_depth() const = 0;

    /** @brief Gets texture size as 3D vector */
    [[nodiscard]] virtual glm::uvec3 get_size() const = 0;

    /** @brief Gets mipmap level count */
    [[nodiscard]] virtual uint32_t get_mip_level_count() const = 0;

    /**
     * @brief Gets dimensions of specific mip level
     * @param mip Mip level index
     * @return Mip size (width/height/depth)
     */
    [[nodiscard]] virtual glm::uvec3 get_mip_size(uint32_t mip) const = 0;

    /**
     * @brief Resizes texture
     * @param size New size
     */
    virtual void resize(const glm::uvec3& size) = 0;

    /**
     * @brief Resizes texture
     * @param width New width
     * @param height New height
     * @param depth New depth
     */
    virtual void resize(size_t width, size_t height, size_t depth) = 0;

    /** @brief Gets underlying image */
    [[nodiscard]] virtual Reference<Image> get_image() const = 0;

    /** @brief Gets CPU buffer (const) */
    [[nodiscard]] virtual Buffer get_buffer() const = 0;

    /** @brief Gets CPU buffer (mutable) */
    virtual Buffer get_writeable_buffer() = 0;

    /** @brief Checks if texture data is loaded */
    virtual bool loaded() const = 0;

    /** @brief Gets texture type (2D/cube) */
    [[nodiscard]] virtual TextureType get_type() const = 0;

    static ResourceType static_type() { return ResourceType::Texture; }
};
}
