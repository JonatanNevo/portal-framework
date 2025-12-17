//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "portal/engine/reference.h"
#include "portal/engine/window/window.h"

namespace portal
{
class GlfwWindow final : public Window
{
public:
    GlfwWindow(const WindowProperties& properties, const CallbackConsumers& consumers);

    ~GlfwWindow() override;

    [[nodiscard]] Reference<renderer::Surface> create_surface(const renderer::vulkan::VulkanContext& context) override;

    void on_event(Event& event) override;
    void process_events() override;

    [[nodiscard]] bool should_close() const override;
    void close() override;

    [[nodiscard]] float get_dpi_factor() const override;

    void maximize() override;
    void center_window() override;

    void set_vsync(bool enable) override;
    void set_resizeable(bool enable) override;
    void set_title(StringId title) override;

    [[nodiscard]] glm::vec2 get_position() const override;

    [[nodiscard]] GLFWwindow* get_handle() const;

private:
    GLFWwindow* handle = nullptr;
};
} // portal
