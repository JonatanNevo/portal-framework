//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window_events.h"
#include "portal/engine/reference.h"
#include "portal/engine/window/window.h"
#include "portal/input/input_events.h"

namespace portal
{
class ProjectSettings;

class GlfwWindow final : public Window
{
public:
    GlfwWindow(ProjectSettings& settings, const WindowProperties& properties, entt::dispatcher& dispatcher);
    ~GlfwWindow() override;

    [[nodiscard]] Reference<renderer::Surface> create_surface(const renderer::vulkan::VulkanContext& context) override;

    void process_events() override;

    [[nodiscard]] bool should_close() const override;
    void close() override;

    [[nodiscard]] float get_dpi_factor() const override;

    void maximize() override;
    void restore() override;
    void minimize() override;
    void center_window() override;
    WindowExtent resize(const WindowExtent& requested_extent) override;

    void set_vsync(bool enable) override;
    void set_resizeable(bool enable) override;
    void set_title(StringId title) override;

    [[nodiscard]] glm::vec2 get_position() const override;

    [[nodiscard]] GLFWwindow* get_handle() const;
    [[nodiscard]] bool is_maximised() const override;
    [[nodiscard]] bool is_minimized() const override;

protected:
    void change_mouse_mode(SetMouseCursorEvent event) const;
    void window_drag(WindowDragEvent event);
    void maximize_or_restore();
    void request_minimize();
    void request_close();
    void inner_update_maximized(WindowUpdateMaximizedEvent event);

private:
    ProjectSettings& settings;
    GLFWwindow* handle = nullptr;

    // GLFW has a bug in macOS regarding when its considers maximized or not when using undecorated windows.
    // Until this is fixed, we are tracking the state ourselves
    bool maximized = false;

#ifdef PORTAL_PLATFORM_MACOS
    int saved_xpos = 0, saved_ypos = 0;
    int saved_width = 0, saved_height = 0;
#endif
};
} // portal
