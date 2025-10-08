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
#include "portal/core/reference.h"

#include "portal/engine/renderer/camera.h"
#include "portal/engine/renderer/deletion_queue.h"
#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/rendering_types.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "vulkan/gpu_context.h"
#include "portal/engine/scene/draw_context.h"
#include "portal/engine/scene/scene.h"

namespace portal
{
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
    vulkan::DescriptorAllocator frame_descriptors;
};

class Renderer
{
public:
    void init(const Ref<renderer::vulkan::VulkanContext>& new_context, renderer::vulkan::VulkanWindow* new_window);
    void cleanup();

    void set_scene(const Ref<Scene>& new_scene);
    std::shared_ptr<renderer::vulkan::GpuContext> get_gpu_context();

    void begin_frame();
    void end_frame();
    void clean_frame();

    void update_imgui(float delta_time);
    void update_scene(float delta_time);

    void draw_geometry();
    void draw_geometry(const vk::raii::CommandBuffer& command_buffer);

    void on_resize(size_t new_width, size_t new_height);

    FrameData& get_current_frame_data();

private:
    void init_swap_chain();
    void init_descriptors();

    void init_gpu_context();

    void immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function);

private:
    Ref<renderer::vulkan::VulkanContext> context;
    renderer::vulkan::VulkanWindow* window = nullptr;

    Ref<renderer::RenderTarget> render_target;

    EngineStats stats = {};

    DeletionQueue deletion_queue = {};

    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;

    vk::raii::Fence immediate_fence = nullptr;
    vk::raii::CommandPool immediate_command_pool = nullptr;
    vk::raii::CommandBuffer immediate_command_buffer = nullptr;
    TracyVkCtx tracy_context = nullptr;

    vulkan::GPUSceneData scene_data{};
    vk::raii::DescriptorSetLayout scene_descriptor_set_layout = nullptr;
    std::vector<FrameData> frame_data;

    std::shared_ptr<renderer::vulkan::GpuContext> gpu_context = nullptr;
    scene::DrawContext draw_context{};
    WeakRef<Scene> scene;
    bool is_initialized = false;

public:
    // TODO: use input class... and resources....
    Camera camera;
};

} // portal
