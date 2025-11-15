//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_window.h"

#include <imgui.h>
#include <stb_image.h>

#include "portal/core/log.h"
#include "portal/engine/events/window_events.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"
#include "portal/input/input_events.h"


namespace portal::renderer::vulkan
{
static auto logger = Log::get_logger("Vulkan");

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

VulkanWindow::VulkanWindow(InputProvider& input, const VulkanContext& context, const WindowSpecification& spec)
    : spec(spec),
      data(
          spec.title,
          spec.width,
          spec.height,
          nullptr,
          input
          ),
      context(context)
{
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
    // TODO: Use resource registry here
    GLFWimage icon;
    icon.pixels = stbi_load(R"(C:\Code\portal-framework\engine\resources\portal_icon_64x64.png)", &icon.width, &icon.height, 0, 4);
    glfwSetWindowIcon(window, 1, &icon);

    swapchain = make_reference<VulkanSwapchain>(context, window);
    swapchain->create(reinterpret_cast<uint32_t*>(&data.width), reinterpret_cast<uint32_t*>(&data.height), spec.vsync);

    // TODO: combine with `input` module
    glfwSetWindowUserPointer(window, &data);

    const bool raw_mouse_motion_supported = glfwRawMouseMotionSupported();
    if (raw_mouse_motion_supported)
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    else
        LOGGER_WARN("Raw mouse motion not supported");

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(
        window,
        [](GLFWwindow* handle, const int width, const int height)
        {
            [[maybe_unused]] auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(handle));

            WindowResizeEvent event(static_cast<size_t>(width), static_cast<size_t>(height));

            data.event_callback(event);
            data.width = event.get_width();
            data.height = event.get_height();
        }
        );

    glfwSetWindowCloseCallback(
        window,
        [](GLFWwindow* handle)
        {
            [[maybe_unused]] const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(handle));

            WindowCloseEvent event;
            data.event_callback(event);
        }
        );

    glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
    glfwSetKeyCallback(
        window,
        [](GLFWwindow* handle, int key, int, int action, int mods)
        {
            const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(handle));
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

            data.input.report_key_action(portal_key, state, modifiers);
        }
        );

    glfwSetMouseButtonCallback(
        window,
        [](GLFWwindow* handle, int button, int action, int)
        {
            const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(handle));
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
            data.input.report_key_action(portal_key, state, std::nullopt);
        }
        );

    glfwSetScrollCallback(
        window,
        [](GLFWwindow* handle, const double x_offset, const double y_offset)
        {
            const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(handle));
            data.input.report_axis_change(Axis::MouseScroll, glm::vec2(static_cast<float>(x_offset), static_cast<float>(y_offset)));
        }
        );

    glfwSetCursorPosCallback(
        window,
        [](GLFWwindow* handle, const double x_pos, const double y_pos)
        {
            const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(handle));
            data.input.report_axis_change(Axis::Mouse, glm::vec2(static_cast<float>(x_pos), static_cast<float>(y_pos)));
        }
        );

    // glfwSetTitlebarHitTestCallback
    // glfwSetWindowIconifyCallback
    // glfwSetCharCallback

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

VulkanWindow::~VulkanWindow()
{
    swapchain->destroy();

    for (const auto& cursor : cursors)
        glfwDestroyCursor(cursor);
}

void VulkanWindow::process_events()
{
    glfwPollEvents();
}

void VulkanWindow::swap_buffers()
{
    swapchain->present();
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

    swapchain->set_vsync(enable);
    swapchain->on_resize(data.width, data.height);
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

VulkanSwapchain& VulkanWindow::get_swapchain() const
{
    return *swapchain;
}

void VulkanWindow::set_event_callback(const std::function<void(Event&)> callback)
{
    data.event_callback = callback;
}

void VulkanWindow::on_event(Event& event)
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

            glfwSetInputMode(window, GLFW_CURSOR, mode);
            return true;
        }
        );
}

GLFWwindow* VulkanWindow::get_glfw_window() const
{
    return window;
}


}
