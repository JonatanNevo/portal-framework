//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <tracy/TracyVulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "portal/core/events/event_handler.h"

#include "portal/engine/renderer/camera.h"
#include "portal/engine/renderer/deletion_queue.h"
#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/renderer/rendering_types.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"
#include "portal/engine/scene/draw_context.h"
#include "portal/engine/scene/scene.h"

namespace portal
{
namespace renderer {
    class RenderTarget;
}

namespace renderer::vulkan {
    class VulkanPipeline;
    class VulkanWindow;
}

class Window;

struct EngineStats
{
    float frame_time;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};

struct FrameData
{
    DeletionQueue deletion_queue = {};

    vk::raii::DescriptorSet global_descriptor_set = nullptr;
    renderer::vulkan::AllocatedBuffer scene_data_buffer = nullptr;
    renderer::vulkan::DescriptorAllocator frame_descriptors;
};

class Renderer final : public EventHandler
{
public:
    Renderer(Input& input, renderer::vulkan::VulkanContext& context, const Reference<renderer::vulkan::VulkanSwapchain>& swapchain);
    ~Renderer() override;

    void cleanup();

    void begin_frame();
    void end_frame();
    void clean_frame();

    void update_imgui(float delta_time);
    void update_scene(float delta_time, ResourceReference<Scene>& scene);

    void draw_geometry();
    void draw_geometry(const vk::raii::CommandBuffer& command_buffer);

    void on_resize(size_t new_width, size_t new_height);

    FrameData& get_current_frame_data();

    [[nodiscard]] const RendererContext& get_renderer_context() const;
    [[nodiscard]] size_t get_frames_in_flight() const;
    [[nodiscard]] const renderer::vulkan::VulkanSwapchain& get_swapchain() const;

    void on_event(Event& event) override;

private:
    void make_render_target();
    void init_descriptors();

    void immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function);

private:
    Reference<renderer::vulkan::VulkanSwapchain> swapchain;
    renderer::vulkan::VulkanContext& context;

    Reference<renderer::RenderTarget> render_target = nullptr;

    EngineStats stats = {};

    DeletionQueue deletion_queue = {};

    // vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;

    vk::raii::Fence immediate_fence = nullptr;
    vk::raii::CommandPool immediate_command_pool = nullptr;
    vk::raii::CommandBuffer immediate_command_buffer = nullptr;
    // TracyVkCtx tracy_context = nullptr;

    renderer::vulkan::GPUSceneData scene_data{};
    std::vector<vk::raii::DescriptorSetLayout> scene_descriptor_set_layouts;
    std::vector<FrameData> frame_data;

    RendererContext renderer_context;
    scene::DrawContext draw_context{};
    bool is_initialized = false;

public:
    // TODO: use input class... and resources....
    Camera camera;
};

} // portal
