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

class Image: public RendererResource
{
public:
    explicit Image(const StringId& id): RendererResource(id) {}

    virtual void resize(size_t width, size_t height) = 0;
    virtual void reallocate() = 0;
    virtual void release() = 0;

    [[nodiscard]] virtual size_t get_width() const = 0;
    [[nodiscard]] virtual size_t get_height() const = 0;
    [[nodiscard]] virtual glm::uvec2 get_size() const = 0;
    [[nodiscard]] virtual bool has_mip() const = 0;

    [[nodiscard]] virtual float get_aspect_ratio() const = 0;
    [[nodiscard]] virtual const image::Properties& get_prop() const = 0;

    [[nodiscard]] virtual Buffer get_buffer() const = 0;
    [[nodiscard]] virtual Buffer& get_buffer() = 0;

    virtual void create_per_layer_image_view() = 0;

    virtual void set_data(Buffer buffer) = 0;
    virtual Buffer copy_to_host_buffer() = 0;
};

struct ImageViewProperties
{
    Reference<Image> image;
    size_t mip = 0;

    StringId name;
};

class ImageView: public RendererResource
{
public:
    explicit ImageView(const StringId& id): RendererResource(id) {}
};

namespace utils
{
    size_t calculate_mip_count(size_t width, size_t height, size_t depth = 1);
    size_t get_image_memory_size(ImageFormat format, size_t width, size_t height, size_t depth = 1);
}
} // portal