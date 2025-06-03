//
// Created by thejo on 5/23/2025.
//

#pragma once
#include <functional>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>

#include "portal/application/layer.h"

namespace portal {
class UIRenderable;
}

namespace portal
{

class Renderer final : public Layer
{
public:
    void on_attach(Application* app) override;
    void on_detach() override;

    void update(float dt) override;

    void add_ui_renderable(const std::shared_ptr<UIRenderable>& ui_renderable);
    void submit_resource_free(std::function<void()>&& func);
    vk::CommandBuffer get_command_buffer();
    void flush_command_buffer(vk::CommandBuffer command_buffer) const;

private:
    void setup_vulkan(std::vector<const char*> extensions);
    void setup_vulkan_window(ImGui_ImplVulkanH_Window* window, vk::SurfaceKHR surface, int width, int height) const;
    void cleanup_vulkan() const;
    void cleanup_vulkan_window();
    void frame_render(ImGui_ImplVulkanH_Window* window, ImDrawData* draw_data);
    void frame_present(ImGui_ImplVulkanH_Window* window);

// TODO: Change to private
public:
    vk::Instance instance;
    vk::PhysicalDevice physical_device;
    vk::Device device;
    uint32_t queue_family = std::numeric_limits<uint32_t>::max();
    vk::Queue queue;
    vk::DebugReportCallbackEXT debug_callback = nullptr;
    vk::PipelineCache pipeline_cache = nullptr;
    vk::DescriptorPool descriptor_pool = nullptr;

    // TODO: Use my own struct
    ImGui_ImplVulkanH_Window window_data;
    int min_image_count = 2;
    bool swap_chain_rebuild = false;

    std::vector<std::vector<vk::CommandBuffer>> allocated_command_buffers;
    std::vector<std::vector<std::function<void()>>> resource_free_queue;

    vk::CommandBuffer active_command_buffer = nullptr;
    uint32_t current_frame_index = 0;

    std::vector<std::shared_ptr<UIRenderable>> ui_renderables;
};

} // portal
