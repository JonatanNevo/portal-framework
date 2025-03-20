//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "windows_platform.h"

#include "portal/application/window/glfw/glfw_window.h"
#include "portal/application/window/headless/headless_window.h"

namespace portal
{
void WindowsPlatform::create_window(const Window::Properties& properties)
{
    if (properties.mode == Window::Mode::Headless)
    {
        window = std::make_unique<HeadlessWindow>(properties);
    }
    else
    {
        window = std::make_unique<GlfwWindow>(this, properties);
    }
}
}

std::unique_ptr<portal::Platform> create_platform()
{
    return std::make_unique<portal::WindowsPlatform>();
}

