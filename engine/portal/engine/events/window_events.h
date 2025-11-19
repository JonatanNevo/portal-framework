//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/events/event.h"


namespace portal
{

class WindowResizeEvent final : public Event
{
public:
    WindowResizeEvent(const size_t width, const size_t height): width(width), height(height) {}

    [[nodiscard]] size_t get_width() const { return width; }
    [[nodiscard]] size_t get_height() const { return height; }

    [[nodiscard]] std::string to_string() const override
    {
        return std::format("{}: {}x{}", get_name().string, width, height);
    }

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(Window)

private:
    size_t width;
    size_t height;
};

class WindowCloseEvent final : public Event
{
public:
    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(Window)
};

}
