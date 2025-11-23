//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "portal/application/frame_context.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/deletion_queue.h"
#include "portal/engine/renderer/render_target/render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/surface/vulkan_surface.h"

namespace portal::renderer::vulkan
{
class VulkanRenderTarget;

struct SwapchainImageData
{
    vk::Image image = nullptr;
    vk::raii::ImageView image_view = nullptr;

    // Track which frame last used this image
    size_t last_used_frame = std::numeric_limits<size_t>::max();
};



class VulkanSwapchain
{
public:
    VulkanSwapchain(const VulkanContext& context, const Reference<Surface>& surface);

    void create(uint32_t* request_width, uint32_t* request_height, bool new_vsync);
    void destroy();

    void on_resize(size_t new_width, size_t new_height);

    SwapchainImageData& begin_frame(const FrameContext& frame);
    void present(const FrameContext& frame);

    [[nodiscard]] size_t get_image_count() const { return images_data.size(); }
    [[nodiscard]] size_t get_width() const { return width; }
    [[nodiscard]] size_t get_height() const { return height; }
    [[nodiscard]] const vk::Format& get_color_format() const { return color_format; }
    [[nodiscard]] vk::ColorSpaceKHR get_color_space() const { return color_space; }

    void set_vsync(const bool new_vsync) { vsync = new_vsync; }

private:
    size_t acquire_next_image(const FrameContext& frame);

    void find_image_format_and_color_space();

private:
    const VulkanContext& context;
    bool vsync = false;

    Reference<VulkanSurface> surface;
    vk::raii::SwapchainKHR swapchain = nullptr;

    size_t width = 0, height = 0;
    vk::Format color_format{};
    vk::ColorSpaceKHR color_space{};

    std::vector<vk::Image> swap_chain_images;
    std::vector<SwapchainImageData> images_data;

    // Index of the current swapchain image. can be different from the frame index
    size_t current_image = 0;

};
} // portal
