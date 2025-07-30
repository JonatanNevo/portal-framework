//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <tracy/TracyVulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <glm/gtx/hash.hpp>

#include "portal/core/buffer.h"
#include "portal/core/timer.h"

namespace portal
{
struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 tex_coord;

    static vk::VertexInputBindingDescription get_binding_description();
    static std::array<vk::VertexInputAttributeDescription, 3> get_attribute_descriptions();

    bool operator==(const Vertex& other) const;
};
}


namespace std
{
template <>
struct hash<portal::Vertex>
{
    size_t operator()(const portal::Vertex& vertex) const
    {
        return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.tex_coord) << 1);
    }
};
}

namespace portal
{
struct GameObject
{
    // Transform properties
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::quat rotation = {};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};

    // Uniform buffer for this object (one per frame in flight)
    std::vector<vk::raii::Buffer> uniform_buffers;
    std::vector<vk::raii::DeviceMemory> uniform_buffers_memory;
    std::vector<void*> uniform_buffers_mapped_ptr;

    // Descriptor sets for this object (one per frame in flight)
    std::vector<vk::raii::DescriptorSet> descriptor_sets;

    [[nodiscard]] glm::mat4 get_model_matrix() const;
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

class OldRenderer
{
public:
    void run();

    static void frame_buffer_size_callback(GLFWwindow* window, int width, int height);

protected:
    void init_window();
    void init_vulkan();
    void main_loop();
    void cleanup();

    void setup_game_objects();

    void create_vma_allocator();
    void create_instance();
    void create_debug_messenger();
    void create_surface();
    void pick_physical_device();
    void create_logical_device();
    void create_swap_chain();
    void create_image_views();
    void create_descriptor_set_layout();
    void create_graphics_pipeline();
    void create_command_pool();
    void create_depth_resources();
    void create_texture_image();
    void create_texture_image_view();
    void create_texture_sampler();
    void load_model();
    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_command_buffers();
    void create_sync_objects();
    void create_color_resources();

    void record_command_buffer(uint32_t image_index);
    void update_uniform_buffer(float dt, uint32_t frame_index);
    void draw_frame(float dt);
    void cleanup_swap_chain();
    void recreate_swap_chain();

private:
    void create_buffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::raii::Buffer& buffer,
        vk::raii::DeviceMemory& buffer_memory
        ) const;
    void copy_buffer(
        const vk::raii::CommandBuffer& command_buffer,
        const vk::raii::Buffer& src,
        const vk::raii::Buffer& dst,
        vk::DeviceSize size
        ) const;
    void copy_buffer_to_image(
        const vk::raii::CommandBuffer& command_buffer,
        const vk::raii::Buffer& buffer,
        const vk::raii::Image& image,
        uint32_t width,
        uint32_t height
        ) const;

    void create_image(
        uint32_t width,
        uint32_t height,
        vk::Format format,
        uint32_t mip_level,
        vk::SampleCountFlagBits samples,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::raii::Image& image,
        vk::raii::DeviceMemory& imageMemory
        ) const;

    [[nodiscard]] vk::raii::ImageView create_image_view(
        const vk::Image& image,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags,
        uint32_t mip_level = 1
        ) const;

    void generate_mipmaps(
        const vk::raii::CommandBuffer& command_buffer,
        const vk::raii::Image& image,
        vk::Format image_format,
        int32_t width,
        int32_t height,
        uint32_t mip_level
        ) const;

    [[nodiscard]] vk::raii::CommandBuffer begin_single_time_commands() const;
    void end_single_time_commands(const vk::raii::CommandBuffer& command_buffer) const;

    [[nodiscard]] vk::Format find_depth_format() const;
    [[nodiscard]] vk::Format find_supported_format(
        const std::vector<vk::Format>& candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features
        ) const;
    [[nodiscard]] std::vector<const char*> get_required_extensions() const;
    [[nodiscard]] std::vector<const char*> get_required_validation_layers() const;
    static uint32_t rate_device_suitability(const vk::raii::PhysicalDevice& device);
    static uint32_t find_queue_families(const vk::raii::PhysicalDevice& device, vk::QueueFlagBits queue_type);
    static vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats);
    static vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes);
    [[nodiscard]] vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    [[nodiscard]] uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) const;
    [[nodiscard]] vk::raii::ShaderModule create_shader_module(const Buffer& code) const;
    [[nodiscard]] vk::SampleCountFlagBits get_max_usable_sample_count() const;
    void transition_image_layout(
        uint32_t image_index,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
        ) const;

    void transition_image_layout(
        const vk::raii::CommandBuffer& command_buffer,
        const vk::raii::Image& image,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        uint32_t mip_level = 1
        ) const;

    void transition_image_layout(
        const vk::raii::CommandBuffer& command_buffer,
        const vk::Image& image,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor,
        uint32_t mip_level = 1
        ) const;

public:
    bool frame_buffer_resized = false;

private:
    GLFWwindow* window = nullptr;
    // VmaAllocator vma_allocator = nullptr;

    vk::raii::Context context{};
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    vk::raii::PhysicalDevice physical_device = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphics_queue = nullptr;
    vk::raii::Queue present_queue = nullptr;

    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::SwapchainKHR swap_chain = nullptr;
    vk::Format swap_chain_image_format = vk::Format::eUndefined;
    vk::Extent2D swap_chain_extent;
    std::vector<vk::Image> swap_chain_images;
    std::vector<vk::raii::ImageView> swap_chain_image_views;

    vk::raii::DescriptorSetLayout descriptor_set_layout = nullptr;
    vk::raii::DescriptorPool descriptor_pool = nullptr;

    vk::raii::PipelineLayout pipeline_Layout = nullptr;
    vk::raii::Pipeline graphics_pipeline = nullptr;

    vk::raii::CommandPool command_pool = nullptr;

    std::unordered_map<Vertex, uint32_t> unique_vertices = {};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vk::raii::Buffer vertex_buffer = nullptr;
    vk::raii::DeviceMemory vertex_buffer_memory = nullptr;
    vk::raii::Buffer index_buffer = nullptr;
    vk::raii::DeviceMemory index_buffer_memory = nullptr;

    uint32_t mip_levels = 0;
    vk::raii::Image texture = nullptr;
    vk::raii::DeviceMemory texture_memory = nullptr;
    vk::raii::ImageView texture_image_view = nullptr;
    vk::raii::Sampler texture_sampler = nullptr;

    vk::raii::Image depth_image = nullptr;
    vk::raii::DeviceMemory depth_image_memory = nullptr;
    vk::raii::ImageView depth_image_view = nullptr;

    vk::raii::Image color_image = nullptr;
    vk::raii::DeviceMemory color_image_memory = nullptr;
    vk::raii::ImageView color_image_view = nullptr;

    std::vector<vk::raii::CommandBuffer> command_buffers;

    std::vector<vk::raii::Semaphore> present_complete_semaphores;
    std::vector<vk::raii::Semaphore> render_finished_semaphores;
    std::vector<vk::raii::Fence> in_flight_fences;
    uint32_t current_frame = 0;
    uint32_t semaphore_index = 0;

    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;

    portal::Timer timer;
    float delta_time = 0.0f;

    std::vector<GameObject> objects;

    TracyVkCtx tracy_context = nullptr;

};
}
