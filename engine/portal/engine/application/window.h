//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/strings/string_id.h"

namespace portal
{

struct WindowSpecification
{
    StringId title = STRING_ID("Portal");
    size_t width = 1600;
    size_t height = 900;

    bool decorated = true; // TODO: ?
    bool fullscreen = false;
    bool vsync = true; // TODO: should this be here?
};

class Window
{
public:
    virtual ~Window() = default;

    virtual void init() = 0;
    virtual void shutdown() = 0;

    virtual void process_events() = 0;

    virtual void swap_buffers() = 0; // TODO: ??

    virtual void maximize() = 0;
    virtual void center_window() = 0;

    [[nodiscard]] virtual size_t get_width() const = 0;
    [[nodiscard]] virtual size_t get_height() const = 0;
    [[nodiscard]] virtual std::pair<size_t, size_t> get_extent() const = 0;
    [[nodiscard]] virtual std::pair<float, float> get_position() const = 0;

    virtual void set_vsync(bool enable) = 0;
    [[nodiscard]] virtual bool is_vsynced() const = 0;

    virtual void set_resizeable(bool enable) = 0;

    virtual void set_title(StringId title) = 0;
    [[nodiscard]] virtual StringId get_title() const = 0;

    // TODO: set event callback?

};

} // portal
