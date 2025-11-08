//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "portal/engine/renderer/deletion_queue.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"

namespace portal::renderer::vulkan
{

struct SwapchainImageData
{
    vk::Image image = nullptr;
    vk::raii::ImageView image_view = nullptr;

    vk::raii::CommandPool command_pool = nullptr;
    vk::raii::CommandBuffer command_buffer = nullptr;

    // Track which frame last used this image
    size_t last_used_frame = std::numeric_limits<size_t>::max();
};

struct FrameInFlightData
{
    // Semaphores to signal that images are available for rendering and that rendering has finished
    vk::raii::Semaphore image_available_semaphore = nullptr;
    vk::raii::Semaphore render_finished_semaphore = nullptr;
    // Fence to signal that command buffers are ready to be reused
    vk::raii::Fence wait_fence = nullptr;

    DeletionQueue deletion_queue = {};
};

class VulkanSwapchain
{
public:
    VulkanSwapchain(const VulkanContext& context, GLFWwindow* window);

    void create(uint32_t* request_width, uint32_t* request_height, bool new_vsync);
    void destroy();

    void on_resize(size_t new_width, size_t new_height);

    void begin_frame();
    void present();

    size_t get_image_count() const { return images_data.size(); }
    size_t get_width() const { return width; }
    size_t get_height() const { return height; }
    const vk::Format& get_color_format() const { return color_format; }
    vk::ColorSpaceKHR get_color_space() const { return color_space; }
    size_t get_current_frame() const { return current_frame; }
    [[nodiscard]] size_t get_frames_in_flight() const { return frames_in_flight; }

    vk::raii::CommandBuffer& get_current_draw_command_buffer() { return images_data[current_image].command_buffer; }
    vk::raii::CommandPool& get_current_draw_command_pool() { return images_data[current_image].command_pool; }
    vk::raii::ImageView& get_current_draw_image_view() { return images_data[current_image].image_view; }
    vk::Image get_current_draw_image() const { return images_data[current_image].image; }

    void set_vsync(const bool new_vsync) { vsync = new_vsync; }

private:
    size_t acquire_next_image();

    void find_image_format_and_color_space();

private:
    const VulkanContext& context;
    bool vsync = false;

    vk::raii::SwapchainKHR swapchain = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    size_t present_queue_family_index = 0;
    vk::Queue present_queue = nullptr;

    size_t width = 0, height = 0;
    vk::Format color_format{};
    vk::ColorSpaceKHR color_space{};


    std::vector<vk::Image> swap_chain_images;
    std::vector<SwapchainImageData> images_data;

    size_t frames_in_flight = 0;
    std::vector<FrameInFlightData> frame_data;

    // Index of the frame we are currently working on, up to max frames in flight
    size_t current_frame = 0;
    // Index of the current swapchain image. can be different from the frame index
    size_t current_image = 0;

};
} // portal
