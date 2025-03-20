//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "headless_window.h"

#include "portal/application/vulkan/instance.h"

namespace portal
{
HeadlessWindow::HeadlessWindow(const Window::Properties& properties): Window(properties) {}

vk::SurfaceKHR HeadlessWindow::create_surface(vulkan::Instance* instance)
{
    return create_surface(instance.get_handle(), VK_NULL_HANDLE);
}

vk::SurfaceKHR HeadlessWindow::create_surface(vk::Instance instance, vk::PhysicalDevice physical_device)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if (instance)
    {
        VkHeadlessSurfaceCreateInfoEXT info{};
        info.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;

        auto err = vkCreateHeadlessSurfaceEXT(instance, &info, VK_NULL_HANDLE, &surface);
        if (err != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create headless surface");
        }
    }

    return surface;
}

bool HeadlessWindow::should_close() { return closed; }

void HeadlessWindow::close() { closed = true; }

float HeadlessWindow::get_dpi_factor() const
{
    // This factor is used for scaling UI elements, so return 1.0f (1 x n = n)
    return 1.0f;
}

std::vector<const char*> HeadlessWindow::get_required_surface_extensions() const
{
    return {VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME};
}
} // portal
