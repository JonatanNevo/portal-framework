//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_window.h"

#include <imgui.h>

#include "portal/core/log.h"
#include "portal/core/debug/assert.h"
#include "portal/engine/events/window_events.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"

namespace portal::renderer::vulkan
{
static auto logger = Log::get_logger("Vulkan");

static bool g_glfw_initialized = false;

static void glfw_error_callback(int error, const char* description)
{
    LOGGER_ERROR("GLFW error {}: {}", error, description);
}

VulkanWindow::VulkanWindow(const Ref<VulkanContext>& context, const WindowSpecification& spec) : spec(spec), context(context)
{
    if (!g_glfw_initialized)
    {
        const auto result = glfwInit();
        PORTAL_ASSERT(result == GLFW_TRUE, "Failed to initialize GLFW");
        glfwSetErrorCallback(glfw_error_callback);

        g_glfw_initialized = true;
    }
}

VulkanWindow::~VulkanWindow()
{
    shutdown();
}

void VulkanWindow::init()
{
    data.title = spec.title;
    data.width = spec.width;
    data.height = spec.height;

    LOGGER_INFO("Creating window {} ({}x{})", spec.title.string, spec.width, spec.height);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!spec.decorated)
    {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }

    if (spec.fullscreen)
    {
        auto* primary_monitor = glfwGetPrimaryMonitor();
        const auto* mode = glfwGetVideoMode(primary_monitor);

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        window = glfwCreateWindow(mode->width, mode->height, data.title.string.data(), nullptr, nullptr);
    }
    else
    {
        window = glfwCreateWindow(static_cast<int>(data.width), static_cast<int>(data.height), data.title.string.data(), nullptr, nullptr);
    }

    // TODO: Set Icon if available?

    swapchain.init(context, window);
    swapchain.create(reinterpret_cast<uint32_t*>(&data.width), reinterpret_cast<uint32_t*>(&data.height), spec.vsync);

    // TODO: combine with `input` module
    glfwSetWindowUserPointer(window, &data);

    const bool raw_mouse_motion_supported = glfwRawMouseMotionSupported();
    if (raw_mouse_motion_supported)
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    else
        LOGGER_WARN("Raw mouse motion not supported");

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(window, [](GLFWwindow* handle, const int width, const int height)
    {
        [[maybe_unused]] auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(handle));

        WindowResizeEvent event(static_cast<size_t>(width), static_cast<size_t>(height));

        data.event_callback(event);
        data.width = event.get_width();
        data.height = event.get_height();
    }
    );

    glfwSetWindowCloseCallback(window, [](GLFWwindow* handle)
    {
        [[maybe_unused]] const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(handle));

        WindowCloseEvent event;
        data.event_callback(event);
    });
    //      glfwSetInputMode(m_Window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
    // glfwSetKeyCallback
    // glfwSetCharCallback
    // glfwSetMouseButtonCallback
    // glfwSetScrollCallback
    // glfwSetCursorPosCallback
    // glfwSetTitlebarHitTestCallback
    // glfwSetWindowIconifyCallback

    cursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    cursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR); // TODO: GLFW doesn't have this.
    cursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    cursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    cursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR); // TODO: GLFW doesn't have this.
    cursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR); // TODO: GLFW doesn't have this.
    cursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

    // Update window size to actual size
    {
        int w_width, w_height;
        glfwGetWindowSize(window, &w_width, &w_height);
        data.width = w_width;
        data.height = w_height;
    }
}

void VulkanWindow::shutdown()
{
    swapchain.destroy();
    glfwTerminate();
    g_glfw_initialized = false;
}

void VulkanWindow::process_events()
{
    glfwPollEvents();
    // TODO: Update input
}

void VulkanWindow::swap_buffers()
{
    swapchain.present();
}

void VulkanWindow::maximize()
{
    glfwMaximizeWindow(window);
}

void VulkanWindow::center_window()
{
    const auto* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const int x = (mode->width / 2) - (mode->width / 2);
    const int y = (mode->height / 2) - (mode->height / 2);
    glfwSetWindowPos(window, x, y);
}

size_t VulkanWindow::get_width() const
{
    return data.width;
}

size_t VulkanWindow::get_height() const
{
    return data.height;
}

std::pair<size_t, size_t> VulkanWindow::get_extent() const
{
    return {data.width, data.height};
}

std::pair<float, float> VulkanWindow::get_position() const
{
    int x, y;
    glfwGetWindowPos(window, &x, &y);
    return {static_cast<float>(x), static_cast<float>(y)};
}

void VulkanWindow::set_vsync(const bool enable)
{
    spec.vsync = enable;

    swapchain.set_vsync(enable);
    swapchain.on_resize(data.width, data.height);
}

bool VulkanWindow::is_vsynced() const
{
    return spec.vsync;
}

void VulkanWindow::set_resizeable(const bool enable)
{
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, enable);
}

void VulkanWindow::set_title(const StringId title)
{
    data.title = title;
    glfwSetWindowTitle(window, title.string.data());
}

StringId VulkanWindow::get_title() const
{
    return data.title;
}

VulkanSwapchain& VulkanWindow::get_swapchain()
{
    return swapchain;
}

void VulkanWindow::set_event_callback(const std::function<void(Event&)> callback)
{
    data.event_callback = callback;
}


}
