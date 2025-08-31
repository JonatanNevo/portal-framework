//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <GLFW/glfw3.h>

#include "portal/core/reference.h"
#include "portal/engine/application/window.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"

namespace portal::renderer::vulkan {
class VulkanSwapchain;
}

namespace portal::renderer::vulkan
{
class VulkanContext;
}

namespace portal::renderer::vulkan
{

class VulkanWindow final : public Window
{
public:
    VulkanWindow(const Ref<VulkanContext>& context, const WindowSpecification& spec);
    ~VulkanWindow() override;

    void init() override;
    void shutdown() override;

    void process_events() override;

    void swap_buffers() override;

    void maximize() override;
    void center_window() override;

    [[nodiscard]] size_t get_width() const override;
    [[nodiscard]] size_t get_height() const override;
    [[nodiscard]] std::pair<size_t, size_t> get_extent() const override;
    [[nodiscard]] std::pair<float, float> get_position() const override;

    void set_vsync(bool enable) override;
    [[nodiscard]] bool is_vsynced() const override;

    void set_resizeable(bool enable) override;

    void set_title(StringId title) override;
    [[nodiscard]] StringId get_title() const override;

    [[nodiscard]] VulkanSwapchain& get_swapchain();

    // TODO: temp!!!
    GLFWwindow* window = nullptr;
private:
    struct WindowData
    {
        StringId title{};
        size_t width, height;
    };

    VulkanSwapchain swapchain;

    GLFWcursor* cursors[9] = {};

    WindowSpecification spec;

    WindowData data{};
    float last_frame_time = 0.f;

    Ref<VulkanContext> context;
};

}
