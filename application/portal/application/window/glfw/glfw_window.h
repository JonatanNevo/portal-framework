//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include "portal/application/window/window.h"
#include <GLFW/glfw3.h>

namespace portal
{
class Platform;

class GlfwWindow final : public Window
{
public:
    explicit GlfwWindow(Platform* platform, const Properties& properties);
    ~GlfwWindow() override;

    vk::SurfaceKHR create_surface(vulkan::Instance* instance) override;
    vk::SurfaceKHR create_surface(vk::Instance instance, vk::PhysicalDevice physical_device) override;
    bool should_close() override;
    void process_events() override;
    void close() override;
    [[nodiscard]] float get_dpi_factor() const override;
    float get_content_scale_factor() const override;
    [[nodiscard]] std::vector<const char*> get_required_surface_extensions() const override;

private:
    GLFWwindow* handle = nullptr;
};
} // portal
