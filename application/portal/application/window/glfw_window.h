//
// Created by thejo on 5/23/2025.
//

#pragma once
#include <GLFW/glfw3.h>

#include "portal/application/window/render_target.h"
#include "portal/application/window/window.h"

namespace portal {

class GLFWWindow final : public Window, public RenderTarget
{
public:
    // Window API
    void initialize(WindowSettings settings) override;
    void shutdown() override;

    void poll_events() override;
    float get_time() const override;

    bool should_close() const override;
    bool is_maximized() const override;

    // Render Target API
    std::vector<const char*> get_required_vulkan_extensions() override;
    vk::SurfaceKHR create_surface(vk::Instance instance) override;
    glm::ivec2 get_framebuffer_size() override;

    //TODO: remove this
    GLFWwindow* get_handle() { return window; }
private:
    GLFWwindow* window = nullptr;
};

} // portal