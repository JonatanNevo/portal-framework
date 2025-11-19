//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "glfw_window.h"

#include <stb_image.h>

#include "portal/core/files/file_system.h"
#include "portal/engine/renderer/vulkan/surface/vulkan_surface.h"
#include "portal/engine/window/window_event_consumer.h"
#include "portal/input/input_event_consumer.h"
#include "portal/input/input_events.h"
#include "portal/input/input_types.h"

namespace portal
{

constexpr static frozen::unordered_map<int, Key, 108> KEY_MAPPING{
    // TODO: By luck there are no conflicts between mouse buttons and regular buttons, I should find a better solution for this
    {GLFW_KEY_UNKNOWN, Key::Invalid},
    {GLFW_MOUSE_BUTTON_1, Key::MouseButton0},
    {GLFW_MOUSE_BUTTON_2, Key::MouseButton1},
    {GLFW_MOUSE_BUTTON_3, Key::MouseButton2},
    {GLFW_MOUSE_BUTTON_4, Key::MouseButton3},
    {GLFW_MOUSE_BUTTON_5, Key::MouseButton4},
    {GLFW_MOUSE_BUTTON_6, Key::MouseButton5},
    {GLFW_KEY_A, Key::A},
    {GLFW_KEY_B, Key::B},
    {GLFW_KEY_C, Key::C},
    {GLFW_KEY_D, Key::D},
    {GLFW_KEY_E, Key::E},
    {GLFW_KEY_F, Key::F},
    {GLFW_KEY_G, Key::G},
    {GLFW_KEY_H, Key::H},
    {GLFW_KEY_I, Key::I},
    {GLFW_KEY_J, Key::J},
    {GLFW_KEY_K, Key::K},
    {GLFW_KEY_L, Key::L},
    {GLFW_KEY_M, Key::M},
    {GLFW_KEY_N, Key::N},
    {GLFW_KEY_O, Key::O},
    {GLFW_KEY_P, Key::P},
    {GLFW_KEY_Q, Key::Q},
    {GLFW_KEY_R, Key::R},
    {GLFW_KEY_S, Key::S},
    {GLFW_KEY_T, Key::T},
    {GLFW_KEY_U, Key::U},
    {GLFW_KEY_V, Key::V},
    {GLFW_KEY_W, Key::W},
    {GLFW_KEY_X, Key::X},
    {GLFW_KEY_Y, Key::Y},
    {GLFW_KEY_Z, Key::Z},
    {GLFW_KEY_0, Key::Zero},
    {GLFW_KEY_1, Key::One},
    {GLFW_KEY_2, Key::Two},
    {GLFW_KEY_3, Key::Three},
    {GLFW_KEY_4, Key::Four},
    {GLFW_KEY_5, Key::Five},
    {GLFW_KEY_6, Key::Six},
    {GLFW_KEY_7, Key::Seven},
    {GLFW_KEY_8, Key::Eight},
    {GLFW_KEY_9, Key::Nine},
    {GLFW_KEY_KP_0, Key::NumpadZero},
    {GLFW_KEY_KP_1, Key::NumpadOne},
    {GLFW_KEY_KP_2, Key::NumpadTwo},
    {GLFW_KEY_KP_3, Key::NumpadThree},
    {GLFW_KEY_KP_4, Key::NumpadFour},
    {GLFW_KEY_KP_5, Key::NumpadFive},
    {GLFW_KEY_KP_6, Key::NumpadSix},
    {GLFW_KEY_KP_7, Key::NumpadSeven},
    {GLFW_KEY_KP_8, Key::NumpadEight},
    {GLFW_KEY_KP_9, Key::NumpadNine},
    {GLFW_KEY_KP_MULTIPLY, Key::Multiply},
    {GLFW_KEY_KP_ADD, Key::Add},
    {GLFW_KEY_KP_SUBTRACT, Key::Subtract},
    {GLFW_KEY_KP_DECIMAL, Key::Decimal},
    {GLFW_KEY_KP_DIVIDE, Key::Divide},
    {GLFW_KEY_LEFT_SHIFT, Key::LeftShift},
    {GLFW_KEY_RIGHT_SHIFT, Key::RightShift},
    {GLFW_KEY_LEFT_CONTROL, Key::LeftControl},
    {GLFW_KEY_RIGHT_CONTROL, Key::RightControl},
    {GLFW_KEY_LEFT_ALT, Key::LeftAlt},
    {GLFW_KEY_RIGHT_ALT, Key::RightAlt},
    {GLFW_KEY_LEFT_SUPER, Key::LeftSystem},
    {GLFW_KEY_RIGHT_SUPER, Key::RightSystem},
    {GLFW_KEY_BACKSPACE, Key::BackSpace},
    {GLFW_KEY_TAB, Key::Tab},
    {GLFW_KEY_ENTER, Key::Enter},
    {GLFW_KEY_PAUSE, Key::Pause},
    {GLFW_KEY_CAPS_LOCK, Key::CapsLock},
    {GLFW_KEY_ESCAPE, Key::Escape},
    {GLFW_KEY_SPACE, Key::SpaceBar},
    {GLFW_KEY_PAGE_UP, Key::PageUp},
    {GLFW_KEY_PAGE_DOWN, Key::PageDown},
    {GLFW_KEY_END, Key::End},
    {GLFW_KEY_HOME, Key::Home},
    {GLFW_KEY_INSERT, Key::Insert},
    {GLFW_KEY_DELETE, Key::Delete},
    {GLFW_KEY_NUM_LOCK, Key::NumLock},
    {GLFW_KEY_SCROLL_LOCK, Key::ScrollLock},
    {GLFW_KEY_LEFT, Key::Left},
    {GLFW_KEY_RIGHT, Key::Right},
    {GLFW_KEY_UP, Key::Up},
    {GLFW_KEY_DOWN, Key::Down},
    {GLFW_KEY_F1, Key::F1},
    {GLFW_KEY_F2, Key::F2},
    {GLFW_KEY_F3, Key::F3},
    {GLFW_KEY_F4, Key::F4},
    {GLFW_KEY_F5, Key::F5},
    {GLFW_KEY_F6, Key::F6},
    {GLFW_KEY_F7, Key::F7},
    {GLFW_KEY_F8, Key::F8},
    {GLFW_KEY_F9, Key::F9},
    {GLFW_KEY_F10, Key::F10},
    {GLFW_KEY_F11, Key::F11},
    {GLFW_KEY_F12, Key::F12},
    {GLFW_KEY_SEMICOLON, Key::Semicolon},
    {GLFW_KEY_EQUAL, Key::Equals},
    {GLFW_KEY_COMMA, Key::Comma},
    {GLFW_KEY_MINUS, Key::Hyphen},
    {GLFW_KEY_PERIOD, Key::Period},
    {GLFW_KEY_SLASH, Key::Slash},
    {GLFW_KEY_GRAVE_ACCENT, Key::Tilde},
    // TODO: this this tilde?
    {GLFW_KEY_LEFT_BRACKET, Key::LeftBracket},
    {GLFW_KEY_RIGHT_BRACKET, Key::RightBracket},
    {GLFW_KEY_BACKSLASH, Key::Backslash},
    {GLFW_KEY_APOSTROPHE, Key::Apostrophe},
    {GLFW_KEY_SEMICOLON, Key::Colon},
    // TODO: there is only semicolon in glfw
    // TODO: missing special characters that require shift to print
    // {GLFW_KEY_, Key::Underscore},
    // {GLFW_KEY_, Key::Ampersand},
    // {GLFW_KEY_, Key::Asterix},
    // {GLFW_KEY_, Key::Caret},
    // {GLFW_KEY_, Key::Dollar},
    // {GLFW_KEY_, Key::Exclamation},
    // {GLFW_KEY_, Key::LeftParantheses},
    // {GLFW_KEY_, Key::RightParantheses},
    // {GLFW_KEY_, Key::Quote},
};

static auto logger = Log::get_logger("GLFW Window");

static void glfw_error_callback(int error, const char* description)
{
    LOGGER_ERROR("GLFW error {}: {}", error, description);
}

static void glfw_window_resize_callback(GLFWwindow* handle, const int width, const int height)
{
    if (const auto* consumers = static_cast<CallbackConsumers*>(glfwGetWindowUserPointer(handle)))
    {
        consumers->window.get().on_resize(WindowExtent(static_cast<size_t>(width), static_cast<size_t>(height)));
    }
}

static void glfw_window_focus_callback(GLFWwindow* handle, const int focused)
{
    if (const auto* consumers = static_cast<CallbackConsumers*>(glfwGetWindowUserPointer(handle)))
    {
        consumers->window.get().on_focus(focused != 0);
    }
}

static void glfw_window_close_callback(GLFWwindow* handle)
{
    if (const auto* consumers = static_cast<CallbackConsumers*>(glfwGetWindowUserPointer(handle)))
    {
        glfwSetWindowShouldClose(handle, GLFW_TRUE);
        consumers->window.get().on_close();
    }
}

static void glfw_key_callback(GLFWwindow* handle, int key, int, int action, int mods)
{
    if (const auto* consumers = static_cast<CallbackConsumers*>(glfwGetWindowUserPointer(handle)))
    {
        auto portal_key = KEY_MAPPING.at(key);


        KeyModifierFlag modifiers = KeyModifierBits::None;
        if (mods & GLFW_MOD_SHIFT)
            modifiers |= KeyModifierBits::Shift;
        if (mods & GLFW_MOD_CONTROL)
            modifiers |= KeyModifierBits::Ctrl;
        if (mods & GLFW_MOD_ALT)
            modifiers |= KeyModifierBits::Alt;
        if (mods & GLFW_MOD_SUPER)
            modifiers |= KeyModifierBits::System;
        if (mods & GLFW_MOD_CAPS_LOCK)
            modifiers |= KeyModifierBits::CapsLock;
        if (mods & GLFW_MOD_NUM_LOCK)
            modifiers |= KeyModifierBits::NumLock;

        KeyState state;
        switch (action)
        {
        case GLFW_PRESS:
            state = KeyState::Pressed;
            break;
        case GLFW_RELEASE:
            state = KeyState::Released;
            break;
        case GLFW_REPEAT:
            state = KeyState::Repeat;
            break;
        default:
            LOGGER_ERROR("Unknown key action: {}", action);
            state = KeyState::Released;
        }

        consumers->input.get().report_key_action(portal_key, state, modifiers);
    }
}

static void glfw_mouse_button_callback(GLFWwindow* handle, int button, int action, int)
{
    if (const auto* consumers = static_cast<CallbackConsumers*>(glfwGetWindowUserPointer(handle)))
    {
        const auto portal_key = KEY_MAPPING.at(button);

        KeyState state;
        switch (action)
        {
        case GLFW_PRESS:
            state = KeyState::Pressed;
            break;
        case GLFW_RELEASE:
            state = KeyState::Released;
            break;
        default:
            LOGGER_ERROR("Unknown mouse button action: {}", action);
            state = KeyState::Released;
        }
        consumers->input.get().report_key_action(portal_key, state, std::nullopt);
    }
}

static void glfw_scroll_callback(GLFWwindow* handle, double x_offset, double y_offset)
{
    if (const auto* consumers = static_cast<CallbackConsumers*>(glfwGetWindowUserPointer(handle)))
    {
        consumers->input.get().report_axis_change(Axis::MouseScroll, glm::vec2(static_cast<float>(x_offset), static_cast<float>(y_offset)));
    }
}

static void glfw_cursor_pos_callback(GLFWwindow* handle, double x_pos, double y_pos)
{
    if (const auto* consumers = static_cast<CallbackConsumers*>(glfwGetWindowUserPointer(handle)))
    {
        consumers->input.get().report_axis_change(Axis::Mouse, glm::vec2(static_cast<float>(x_pos), static_cast<float>(y_pos)));
    }
}

GlfwWindow::GlfwWindow(const WindowProperties& properties, const CallbackConsumers& consumers) : Window(properties, consumers)
{
    LOGGER_INFO("Creating window {} ({}x{})", properties.title.string, properties.extent.width, properties.extent.height);

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwSetErrorCallback(glfw_error_callback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!properties.decorated)
    {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }

    // Create Handle
    // TODO: Allow window recreation on mode change
    switch (properties.mode)
    {
    case WindowMode::Fullscreen:
    {
        // TODO: get window monitor from settings
        auto* monitor = glfwGetPrimaryMonitor();
        auto* mode = glfwGetVideoMode(monitor);
        handle = glfwCreateWindow(mode->width, mode->height, properties.title.string.data(), monitor, nullptr);
        break;
    }
    case WindowMode::FullscreenBorderless:
    {
        auto* monitor = glfwGetPrimaryMonitor();
        auto* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        handle = glfwCreateWindow(mode->width, mode->height, properties.title.string.data(), monitor, nullptr);
        break;
    }
    default:
        handle = glfwCreateWindow(
            static_cast<int>(properties.extent.width),
            static_cast<int>(properties.extent.height),
            properties.title.string.data(),
            nullptr,
            nullptr
            );
        break;
    }

    if (!handle)
    {
        throw std::runtime_error("Couldn't create glfw window.");
    }

    // Setup icon
    if (FileSystem::exists(properties.icon_path))
    {
        GLFWimage icon;
        icon.pixels = stbi_load(properties.icon_path.string().c_str(), &icon.width, &icon.height, 0, 4);
        glfwSetWindowIcon(handle, 1, &icon);
    }
    else
    {
        LOGGER_WARN("Icon file {} does not exist", properties.icon_path.generic_string());
    }

    glfwSetWindowUserPointer(handle, &this->consumers);

    const bool raw_mouse_motion_supported = glfwRawMouseMotionSupported();
    if (raw_mouse_motion_supported)
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    else
        LOGGER_WARN("Raw mouse motion not supported");

    glfwSetWindowCloseCallback(handle, glfw_window_close_callback);
    glfwSetWindowSizeCallback(handle, glfw_window_resize_callback);
    glfwSetWindowFocusCallback(handle, glfw_window_focus_callback);
    glfwSetKeyCallback(handle, glfw_key_callback);
    glfwSetMouseButtonCallback(handle, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(handle, glfw_cursor_pos_callback);
    glfwSetScrollCallback(handle, glfw_scroll_callback);

    glfwSetInputMode(handle, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

}

GlfwWindow::~GlfwWindow()
{
    // TODO: support multiple windows?
    glfwTerminate();
}

Reference<renderer::Surface> GlfwWindow::create_surface(const renderer::vulkan::VulkanContext& context)
{
    // TODO: change to create swapchain?
    renderer::SurfaceProperties props
    {
        .debug_name = STRING_ID("Window Surface"),
        .window = std::reference_wrapper{*this}
    };
    return make_reference<renderer::vulkan::VulkanSurface>(context, props);
    // swapchain = make_reference<VulkanSwapchain>(context, window);
    // swapchain->create(reinterpret_cast<uint32_t*>(&data.width), reinterpret_cast<uint32_t*>(&data.height), properties.vsync);
}

void GlfwWindow::process_events()
{
    glfwPollEvents();
}

bool GlfwWindow::should_close() const
{
    return glfwWindowShouldClose(handle);
}

void GlfwWindow::close()
{
    glfwSetWindowShouldClose(handle, GLFW_TRUE);
}

/**
 * It calculates the dpi factor using the density from GLFW physical size
 * <a href="https://www.glfw.org/docs/latest/monitor_guide.html#monitor_size">GLFW docs for dpi</a>
 */
float GlfwWindow::get_dpi_factor() const
{
    const auto primary_monitor = glfwGetPrimaryMonitor();
    const auto mode = glfwGetVideoMode(primary_monitor);

    int width_mm, height_mm;
    glfwGetMonitorPhysicalSize(primary_monitor, &width_mm, &height_mm);

    // As suggested by the GLFW monitor guide
    static constexpr float inch_to_mm = 25.0f;
    static constexpr float win_base_density = 96.0f;

    const auto dpi = static_cast<uint32_t>(mode->width / (width_mm / inch_to_mm));
    const auto dpi_factor = dpi / win_base_density;
    return dpi_factor;
}

void GlfwWindow::maximize()
{
    glfwMaximizeWindow(handle);
}

void GlfwWindow::center_window()
{
    const auto* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const int x = (mode->width / 2) - (mode->width / 2);
    const int y = (mode->height / 2) - (mode->height / 2);
    glfwSetWindowPos(handle, x, y);
}

void GlfwWindow::set_vsync(const bool enable)
{
    properties.vsync = enable;
    // TODO: mark swapchain as invalid for recreation

    // swapchain->set_vsync(enable);
    // swapchain->on_resize(data.width, data.height);
}

void GlfwWindow::set_resizeable(const bool enable)
{
    glfwSetWindowAttrib(handle, GLFW_RESIZABLE, enable);
}

void GlfwWindow::set_title(const StringId title)
{
    properties.title = title;
    glfwSetWindowTitle(handle, title.string.data());
}

glm::vec2 GlfwWindow::get_position() const
{
    int x, y;
    glfwGetWindowPos(handle, &x, &y);
    return {static_cast<float>(x), static_cast<float>(y)};
}

GLFWwindow* GlfwWindow::get_handle() const
{
    return handle;
}

void GlfwWindow::on_event(Event& event)
{
    EventRunner runner(event);
    runner.run_on<SetMouseCursorEvent>(
        [this](const SetMouseCursorEvent& e)
        {
            int mode;
            switch (e.get_mode())
            {
            case CursorMode::Normal:
                mode = GLFW_CURSOR_NORMAL;
                break;
            case CursorMode::Hidden:
                mode = GLFW_CURSOR_HIDDEN;
                break;
            case CursorMode::Locked:
                mode = GLFW_CURSOR_DISABLED;
                break;
            default:
                LOGGER_ERROR("Unknown cursor mode: {}", static_cast<int>(e.get_mode()));
                mode = GLFW_CURSOR_NORMAL;
            }

            glfwSetInputMode(handle, GLFW_CURSOR, mode);
            return true;
        }
        );
}

} // portal
