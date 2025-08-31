//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resources/resource.h"
#include "image_types.h"
#include "portal/core/buffer.h"
#include "portal/engine/renderer/RendererResource.h"

namespace portal::renderer
{

class Image: public RendererResource
{
public:
    virtual void resize(size_t width, size_t height) = 0;
    virtual void initialize() = 0; // TODO: ??
    virtual void release() = 0; // TODO: ??

    [[nodiscard]] virtual size_t get_width() const = 0;
    [[nodiscard]] virtual size_t get_height() const = 0;
    [[nodiscard]] virtual glm::uvec2 get_size() const = 0;
    [[nodiscard]] virtual bool has_mip() const = 0;

    [[nodiscard]] virtual float get_aspect_ratio() const = 0;
    [[nodiscard]] virtual const image::Specification& get_spec() const = 0;

    [[nodiscard]] virtual Buffer get_buffer() const = 0;
    [[nodiscard]] virtual Buffer& get_buffer() = 0;

    virtual void create_per_layer_image_view() = 0;

    virtual void set_data(Buffer buffer) = 0;
    virtual Buffer copy_to_host_buffer() = 0;
};

struct ImageViewSpecification
{
    Ref<Image> image;
    size_t mip = 0;

    StringId name;
};

class ImageView: public RendererResource
{};

namespace utils
{
    size_t calculate_mip_count(size_t width, size_t height);
    size_t get_image_memory_size(ImageFormat format, size_t width, size_t height);
}
} // portal