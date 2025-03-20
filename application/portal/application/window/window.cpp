//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "window.h"

namespace portal
{
Window::Window(Properties properties): properties(std::move(properties)) {}

Window::Extent Window::resize(const Extent& extent)
{
    if (properties.resizable)
    {
        properties.extent = {.width = extent.width, .height = extent.height};
    }
    return properties.extent;
}


void Window::process_events() {}

float Window::get_content_scale_factor() const { return 1.f; }


bool Window::get_display_present_info(vk::DisplayPresentInfoKHR* info, uint32_t src_width, uint32_t src_height) const
{
    // By default, does not support any extra feature
    return false;
}

const Window::Extent& Window::get_extent() const { return properties.extent; }

Window::Mode Window::get_window_mode() const { return properties.mode; }
} // portal
