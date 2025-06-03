//
// Created by thejo on 5/23/2025.
//

#include "glfw_window.h"
#include "portal/core/log.h"

static void glfw_error_callback(int error, const char* description) { fprintf(stderr, "Glfw Error %d: %s\n", error, description); }

namespace portal
{
void GLFWWindow::initialize(const WindowSettings settings)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        LOG_CORE_ERROR_TAG("GLFW", "Failed to initialize GLFW");
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // TODO: Have this be configurable in settings
    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

    int monitor_x, monitor_y;
    glfwGetMonitorPos(primary_monitor, &monitor_x, &monitor_y);

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(settings.width, settings.height, settings.title.c_str(), nullptr, nullptr);

    if (settings.center_window)
    {
        glfwSetWindowPos(window, monitor_x + (mode->width - settings.width) / 2, monitor_y + (mode->height - settings.height) / 2);
        glfwSetWindowAttrib(window, GLFW_RESIZABLE, settings.resizeable ? GLFW_TRUE : GLFW_FALSE);
    }

    glfwShowWindow(window);

    if (!glfwVulkanSupported())
    {
        LOG_CORE_ERROR_TAG("GLFW", "Vulkan not supported!");
        return;
    }

    glfwSetWindowUserPointer(window, this);
}

void GLFWWindow::shutdown()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GLFWWindow::poll_events()
{
    glfwPollEvents();
}

float GLFWWindow::get_time() const
{
    return static_cast<float>(glfwGetTime());
}

bool GLFWWindow::should_close() const
{
    return glfwWindowShouldClose(window);
}

bool GLFWWindow::is_maximized() const
{
    return glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
}

std::vector<const char*> GLFWWindow::get_required_vulkan_extensions()
{
    uint32_t extension_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    std::vector<const char*> result(extension_count);
    for (uint32_t i = 0; i < extension_count; ++i)
    {
        result[i] = extensions[i];
    }
    return result;
}

vk::SurfaceKHR GLFWWindow::create_surface(const vk::Instance instance)
{
    VkSurfaceKHR surface;
    const auto err = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (err != VK_SUCCESS)
    {
        LOG_CORE_ERROR_TAG("GLFW", "Failed to create window surface");
        return VK_NULL_HANDLE;
    }
    return surface;
}

glm::ivec2 GLFWWindow::get_framebuffer_size()
{
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    return {w, h};
}
} // portal
