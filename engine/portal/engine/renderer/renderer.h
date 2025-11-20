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
#include "frame_context.h"
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

class Renderer final : public EventHandler
{
public:
    Renderer(Input& input, renderer::vulkan::VulkanContext& context, const Reference<renderer::vulkan::VulkanSwapchain>& swapchain);
    ~Renderer() override;

    void cleanup();

    renderer::FrameContext begin_frame();
    void end_frame(const renderer::FrameContext& frame);

    void update_imgui(float delta_time);
    void update_scene( renderer::FrameContext& frame, ResourceReference<Scene>& scene);

    void draw_geometry( renderer::FrameContext& frame);
    void draw_geometry(renderer::FrameContext& frame, const vk::raii::CommandBuffer& command_buffer);

    void on_resize(size_t new_width, size_t new_height);

    [[nodiscard]] const RendererContext& get_renderer_context() const;
    [[nodiscard]] size_t get_frames_in_flight() const;
    [[nodiscard]] const renderer::vulkan::VulkanSwapchain& get_swapchain() const;

    void on_event(Event& event) override;

    void update_frame_time(float frame_time);

private:
    void init_render_target();
    void init_frame_resources();

    void immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function);

    static void clean_frame(const renderer::FrameContext& frame);

private:
    Reference<renderer::vulkan::VulkanSwapchain> swapchain;
    renderer::AttachmentProperties attachments;
    renderer::vulkan::VulkanContext& context;

    // TODO: move from here to some `g buffer` class
    Reference<renderer::vulkan::VulkanImage> depth_image;
    Reference<renderer::vulkan::VulkanRenderTarget> render_target;

    EngineStats stats = {};

    DeletionQueue deletion_queue = {};

    // vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;

    vk::raii::Fence immediate_fence = nullptr;
    vk::raii::CommandPool immediate_command_pool = nullptr;
    vk::raii::CommandBuffer immediate_command_buffer = nullptr;
    // TracyVkCtx tracy_context = nullptr;

    renderer::vulkan::GPUSceneData scene_data{};
    std::vector<vk::raii::DescriptorSetLayout> scene_descriptor_set_layouts;
    std::vector<renderer::FrameResources> frames;

    RendererContext renderer_context;
    bool is_initialized = false;

    size_t frames_in_flight = 0;

    // Index of the frame we are currently working on, up to max frames in flight
    size_t current_frame = 0;

public:
    // TODO: use input class... and resources....
    Camera camera;
};

} // portal
