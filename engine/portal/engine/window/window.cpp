//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "window.h"

namespace portal
{
Window::Window(const WindowProperties& properties, entt::dispatcher& dispatcher) : dispatcher(dispatcher), properties(properties) {}

WindowExtent Window::resize(const WindowExtent& requested_extent)
{
    if (properties.resizeable)
    {
        properties.extent.width = std::max(requested_extent.width, properties.minimum_extent.width);
        properties.extent.height = std::max(requested_extent.height, properties.minimum_extent.height);
    }

    return properties.extent;
}

float Window::get_content_scale_factor() const
{
    return 1.0f;
}
}
