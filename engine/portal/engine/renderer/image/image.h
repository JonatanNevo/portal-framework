//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "image_types.h"
#include "portal/core/buffer.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/renderer_resource.h"

namespace portal::renderer
{
class ImageView;

/**
 * @class Image
 * @brief Abstract image interface for GPU textures and render targets
 *
 * Provides dimension queries, data transfer, mipmap support, and per-layer view creation.
 * Concrete implementations (VulkanImage) handle actual GPU allocation.
 */
class Image : public RendererResource
{
public:
    explicit Image(const StringId& id) : RendererResource(id) {}

    /** @brief Resizes image (recreates GPU allocation) */
    virtual void resize(size_t width, size_t height) = 0;

    /** @brief Reallocates image with current properties */
    virtual void reallocate() = 0;

    /** @brief Releases GPU resources */
    virtual void release() = 0;

    /** @brief Gets image width */
    [[nodiscard]] virtual size_t get_width() const = 0;

    /** @brief Gets image height */
    [[nodiscard]] virtual size_t get_height() const = 0;

    /** @brief Gets image size as 2D vector */
    [[nodiscard]] virtual glm::uvec2 get_size() const = 0;

    /** @brief Checks if image has mipmaps */
    [[nodiscard]] virtual bool has_mip() const = 0;

    /** @brief Gets aspect ratio (width/height) */
    [[nodiscard]] virtual float get_aspect_ratio() const = 0;

    /** @brief Gets image properties */
    [[nodiscard]] virtual const image::Properties& get_prop() const = 0;

    /** @brief Gets CPU buffer (const) */
    [[nodiscard]] virtual Buffer get_buffer() const = 0;

    /** @brief Gets CPU buffer (mutable) */
    [[nodiscard]] virtual Buffer& get_buffer() = 0;

    [[nodiscard]] virtual Reference<ImageView> get_view() const = 0;

    /** @brief Creates per-layer image views for array/cube textures */
    virtual void create_per_layer_image_view() = 0;

    /**
     * @brief Uploads data to GPU
     * @param buffer CPU buffer to upload
     */
    virtual void set_data(Buffer buffer) = 0;

    /**
     * @brief Downloads GPU data to CPU buffer
     * @return CPU buffer with image data
     */
    virtual Buffer copy_to_host_buffer() = 0;
};

/**
 * @struct ImageViewProperties
 * @brief Image view creation parameters
 */
struct ImageViewProperties
{
    // TODO: switch to invasive ref counting to be able to save a reference here
    Image* image;
    size_t mip = 0;
    size_t layer = 0;

    StringId name{};
};

/**
 * @class ImageView
 * @brief Abstract image view interface for specific mip/layer access
 */
class ImageView : public RendererResource
{
public:
    explicit ImageView(const ImageViewProperties& properties) : RendererResource(id), properties(properties)
    {
        PORTAL_ASSERT(properties.image != nullptr, "Image cannot be nullptr");
        if (properties.name == INVALID_STRING_ID)
            this->properties.name = properties.image->get_id();
    }

    [[nodiscard]] Image* get_image() const { return properties.image; }
    [[nodiscard]] size_t get_mip() const { return properties.mip; }

protected:
    ImageViewProperties properties;
};

namespace utils
{
    /**
     * @brief Calculates mipmap count for image dimensions
     * @param width Image width
     * @param height Image height
     * @param depth Image depth (default 1)
     * @return Number of mip levels
     */
    size_t calculate_mip_count(size_t width, size_t height, size_t depth = 1);

    /**
     * @brief Calculates image memory size
     * @param format Image format
     * @param width Image width
     * @param height Image height
     * @param depth Image depth (default 1)
     * @return Memory size in bytes
     */
    size_t get_image_memory_size(ImageFormat format, size_t width, size_t height, size_t depth = 1);
}
} // portal
