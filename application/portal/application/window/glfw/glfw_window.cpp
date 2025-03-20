//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "glfw_window.h"


#include <portal/core/log.h>

#include "glfw_input_handlers.h"
#include "portal/application/platform/platform.h"
#include "portal/application/vulkan/instance.h"

namespace portal
{
void error_callback(int error, const char* description)
{
    LOG_CORE_ERROR_TAG("Window", "GLFW Error (code {}): {}", error, description);
}

void window_close_callback(GLFWwindow* window)
{
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void window_size_callback(GLFWwindow* window, const int width, const int height)
{
    if (const auto platform = static_cast<Platform*>(glfwGetWindowUserPointer(window)))
        platform->resize(width, height);
}

void window_focus_callback(GLFWwindow* window, int focused)
{
    if (const auto platform = static_cast<Platform*>(glfwGetWindowUserPointer(window)))
        platform->set_focus(focused);
}

void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    const KeyCode key_code = translate_key_code(key);
    const KeyAction key_action = translate_key_action(action);

    if (const auto platform = static_cast<Platform*>(glfwGetWindowUserPointer(window)))
        platform->input_event(KeyInputEvent{key_code, key_action});
}

void cursor_position_callback(GLFWwindow* window, const double xpos, const double ypos)
{
    if (auto* platform = static_cast<Platform*>(glfwGetWindowUserPointer(window)))
    {
        platform->input_event(
            MouseButtonInputEvent{
                MouseButton::Unknown,
                MouseAction::Move,
                static_cast<float>(xpos),
                static_cast<float>(ypos)
            }
        );
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    const MouseAction mouse_action = translate_mouse_action(action);

    if (auto* platform = static_cast<Platform*>(glfwGetWindowUserPointer(window)))
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        platform->input_event(
            MouseButtonInputEvent{
                translate_mouse_button(button),
                mouse_action,
                static_cast<float>(xpos),
                static_cast<float>(ypos)
            }
        );
    }
}


GlfwWindow::GlfwWindow(Platform* platform, const Properties& properties): Window(properties)
{
    if (!glfwInit())
        throw std::runtime_error("GLFW couldn't be initialized.");

    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // TODO: choose monitor for fullscreen
    switch (properties.mode)
    {
    case Mode::Fullscreen:
        {
            auto* monitor = glfwGetPrimaryMonitor();
            const auto* mode = glfwGetVideoMode(monitor);
            handle = glfwCreateWindow(mode->width, mode->height, properties.title.c_str(), monitor, NULL);
            break;
        }

    case Mode::FullscreenBorderless:
        {
            auto* monitor = glfwGetPrimaryMonitor();
            const auto* mode = glfwGetVideoMode(monitor);
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            handle = glfwCreateWindow(mode->width, mode->height, properties.title.c_str(), monitor, NULL);
            break;
        }

    case Mode::FullscreenStretch:
        {
            throw std::runtime_error("Cannot support stretch mode on this platform.");
            break;
        }

    default:
        handle = glfwCreateWindow(properties.extent.width, properties.extent.height, properties.title.c_str(), NULL, NULL);
        break;
    }

    resize(Extent{properties.extent.width, properties.extent.height});
    if (!handle)
        throw std::runtime_error("Couldn't create glfw window.");

    glfwSetWindowUserPointer(handle, platform);

    glfwSetWindowCloseCallback(handle, window_close_callback);
    glfwSetWindowSizeCallback(handle, window_size_callback);
    glfwSetWindowFocusCallback(handle, window_focus_callback);
    glfwSetKeyCallback(handle, key_callback);
    glfwSetCursorPosCallback(handle, cursor_position_callback);
    glfwSetMouseButtonCallback(handle, mouse_button_callback);

    glfwSetInputMode(handle, GLFW_STICKY_KEYS, 1);
    glfwSetInputMode(handle, GLFW_STICKY_MOUSE_BUTTONS, 1);
}

GlfwWindow::~GlfwWindow()
{
    glfwTerminate();
}

vk::SurfaceKHR GlfwWindow::create_surface(vulkan::Instance* instance)
{
    return create_surface(instance.get_handle(), VK_NULL_HANDLE);
}


vk::SurfaceKHR GlfwWindow::create_surface(vk::Instance instance, vk::PhysicalDevice physical_device)
{
    if (instance == VK_NULL_HANDLE || !handle)
        return VK_NULL_HANDLE;

    VkSurfaceKHR surface;
    const auto ret = glfwCreateWindowSurface(instance, handle, nullptr, &surface);
    if (ret != VK_SUCCESS)
        return nullptr;

    return surface;
}

bool GlfwWindow::should_close()
{
    return glfwWindowShouldClose(handle);
}

void GlfwWindow::process_events()
{
    glfwPollEvents();
}

void GlfwWindow::close()
{
    glfwSetWindowShouldClose(handle, GLFW_TRUE);
}

/// @brief It calculates the dpi factor using the density from GLFW physical size
/// <a href="https://www.glfw.org/docs/latest/monitor_guide.html#monitor_size">GLFW docs for dpi</a>
float GlfwWindow::get_dpi_factor() const
{
    const auto primary_monitor = glfwGetPrimaryMonitor();
    const auto vidmode = glfwGetVideoMode(primary_monitor);

    int width_mm, height_mm;
    glfwGetMonitorPhysicalSize(primary_monitor, &width_mm, &height_mm);

    // As suggested by the GLFW monitor guide
    static constexpr float inch_to_mm = 25.0f;
    static constexpr float win_base_density = 96.0f;

    const auto dpi = static_cast<uint32_t>(vidmode->width / (width_mm / inch_to_mm));
    const auto dpi_factor = dpi / win_base_density;
    return dpi_factor;
}

float GlfwWindow::get_content_scale_factor() const
{
    int fb_width, fb_height;
    glfwGetFramebufferSize(handle, &fb_width, &fb_height);
    int win_width, win_height;
    glfwGetWindowSize(handle, &win_width, &win_height);

    // We could return a 2D result here instead of a scalar,
    // but non-uniform scaling is very unlikely, and would
    // require significantly more changes in the IMGUI integration
    return static_cast<float>(fb_width) / win_width;
}

std::vector<const char*> GlfwWindow::get_required_surface_extensions() const
{
    uint32_t glfw_extension_count{0};
    const char** names = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    return {names, names + glfw_extension_count};
}
} // portal
