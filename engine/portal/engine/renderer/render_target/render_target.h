//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/reference.h"
#include "portal/engine/renderer/render_target/render_target_types.h"

namespace portal::renderer
{
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

    [[nodiscard]] virtual const render_target::Specification& get_spec() const = 0;
};
} // portal