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

#include "portal/core/timer.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/renderer/camera.h"
#include "portal/engine/renderer/deletion_queue.h"
#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/loader.h"
#include "portal/engine/renderer/rendering_types.h"
#include "portal/engine/renderer/scene/draw_context.h"
#include "portal/engine/renderer/scene/gltf_scene.h"
#include "portal/engine/renderer/scene/materials/gltf_metallic_roughness.h"

namespace portal
{

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
    vk::raii::CommandPool command_pool = nullptr;
    vk::raii::CommandBuffer command_buffer = nullptr;
    vk::raii::Semaphore swap_chain_semaphore = nullptr;
    vk::raii::Semaphore render_semaphore = nullptr;
    vk::raii::Fence render_fence = nullptr;

    DeletionQueue deletion_queue = {};

    vk::raii::DescriptorSet global_descriptor_set = nullptr;
    vulkan::AllocatedBuffer scene_data_buffer = nullptr;

    vulkan::DescriptorAllocator frame_descriptors;

    TracyVkCtx tracy_context = nullptr;

    std::array<std::string, 3> debug_name_containers;
};

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct RendererSettings
{
    int width = -1;
    int height = -1;
    std::string title = "Portal Renderer";
};

class Renderer
{
public:
    void init(const RendererSettings& renderer_settings = {});
    void run();
    void cleanup();

    void populate_image(const void* data, vulkan::AllocatedImage& image);
    void generate_mipmaps(const vk::raii::CommandBuffer& command_buffer, const vulkan::AllocatedImage& image);
    vulkan::GPUMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<vulkan::Vertex> vertices);

protected:
    void draw(float delta_time);
    void update_scene(float delta_time);
    void draw_geometry(const vk::raii::CommandBuffer& command_buffer);
    void draw_imgui(const vk::raii::CommandBuffer& command_buffer, const vk::raii::ImageView& target_image_view) const;

    FrameData& get_current_frame_data();

    void resize_swap_chain();

private:
    void init_window();

    void init_vulkan();
    void init_swap_chain();
    void init_commands();
    void init_sync_structures();
    void init_descriptors();
    void init_pipelines();

    void init_default_data();

    void init_imgui();

    void create_swap_chain(uint32_t width, uint32_t height);

    void immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function);

private:
    RendererSettings settings = {};
    EngineStats stats = {};
    vk::Extent2D window_extent = {};

    bool is_initialized = false;
    //bool stop_rendering = false;
    int frame_number = 0;
    Timer timer;

    DeletionQueue deletion_queue = {};

    GLFWwindow* window = nullptr;

    vk::raii::Context context{};
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    vk::raii::PhysicalDevice physical_device = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;

    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;
    vk::raii::SwapchainKHR swap_chain = nullptr;
    vk::Format swap_chain_image_format = vk::Format::eUndefined;

    std::vector<vk::Image> swap_chain_images;
    std::vector<vk::raii::ImageView> swap_chain_image_views;
    vk::Extent2D swap_chain_extent = {};
    vulkan::AllocatedImage draw_image{};
    vulkan::AllocatedImage depth_image{};
    bool resize_requested = false;

    std::vector<FrameData> frame_data{MAX_FRAMES_IN_FLIGHT};
    vk::raii::Queue graphics_queue = nullptr;
    uint32_t graphics_family_index = 0;
    vk::raii::Queue present_queue = nullptr;
    uint32_t present_family_index = 0;

    vk::Extent2D draw_extent = {};
    float render_scale = 1.0f;

    vk::raii::Fence immediate_fence = nullptr;
    vk::raii::CommandPool immediate_command_pool = nullptr;
    vk::raii::CommandBuffer immediate_command_buffer = nullptr;
    TracyVkCtx tracy_context = nullptr;

    vulkan::GPUSceneData scene_data{};
    vk::raii::DescriptorSetLayout scene_descriptor_set_layout = nullptr;

    DrawContext draw_context{};
    // std::unordered_map<std::string, std::shared_ptr<SceneNode>> loaded_nodes;
    // std::vector<std::shared_ptr<vulkan::MeshAsset>> test_meshes;

    std::unordered_map<std::string, std::shared_ptr<GLTFScene>> loaded_scenes;

public:
    // TODO: use input class... and resources....
    Camera camera;

    vulkan::AllocatedImage white_image{};
    vulkan::AllocatedImage black_image{};
    vulkan::AllocatedImage grey_image{};
    vulkan::AllocatedImage error_checker_board_image{};

    vk::raii::Sampler default_sampler_linear = nullptr;
    vk::raii::Sampler default_sampler_nearest = nullptr;

    GLTFMetallicRoughness metal_rough_material{};
};

} // portal
