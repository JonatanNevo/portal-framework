//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "window.h"

namespace portal
{
Window::Window(const WindowProperties& properties, entt::dispatcher& dispatcher) : properties(properties), dispatcher(dispatcher) {}

WindowExtent Window::resize(const WindowExtent& requested_extent)
{
    if (properties.resizeable)
    {
        properties.extent.width = requested_extent.width;
        properties.extent.height = requested_extent.height;
    }

    return properties.extent;
}

float Window::get_content_scale_factor() const
{
    return 1.0f;
}
}
