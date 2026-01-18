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

/**
 * @struct SwapchainImageData
 * @brief Per-swapchain-image data including image handle, view, and last frame index
 *
 * Tracks which frame-in-flight last rendered to this swapchain image. This prevents
 * rendering to an image that's still in-flight from a previous frame, which is critical
 * when frames_in_flight != swapchain_image_count.
 */
struct SwapchainImageData
{
    vk::Image image = nullptr;

    vk::raii::ImageView linear_image_view = nullptr;
    vk::raii::ImageView non_linear_image_view = nullptr;
    size_t last_used_frame = std::numeric_limits<size_t>::max();

    Reference<VulkanRenderTarget> render_target_linear;
    Reference<VulkanRenderTarget> render_target_non_linear;

    // Semaphore signaled when this specific image finishes rendering
    vk::raii::Semaphore render_finished_semaphore = nullptr;
};


/**
 * @class VulkanSwapchain
 * @brief Vulkan swapchain for presentation with per-image tracking and vsync support
 *
 * Manages swapchain creation, acquisition, and presentation. Maintains per-image data
 * (SwapchainImageData) to track which frame-in-flight last used each image, preventing
 * rendering to in-flight images.
 *
 * ## Frame Synchronization
 *
 * begin_frame() acquires next image and returns SwapchainImageData for that image.
 * If that image was used by a previous frame, the caller must wait for that frame's
 * fence before rendering.
 *
 * present() submits the image for presentation, waiting on the frame's render_finished_semaphore.
 *
 * ## Resize Handling
 *
 * on_resize() recreates the swapchain with new dimensions, destroying old resources
 * and creating new image views.
 */
class VulkanSwapchain
{
public:
    /**
     * @brief Constructs swapchain (does not create yet)
     * @param context Vulkan context
     * @param surface Presentation surface
     */
    VulkanSwapchain(VulkanContext& context, const Reference<Surface>& surface);

    /**
     * @brief Creates swapchain with requested dimensions and vsync
     * @param request_width Requested width (may be adjusted to surface capabilities)
     * @param request_height Requested height (may be adjusted to surface capabilities)
     * @param new_vsync Whether vsync is enabled
     */
    void create(uint32_t* request_width, uint32_t* request_height, bool new_vsync);

    /**
     * @brief Destroys swapchain and image views
     */
    void destroy();

    /**
     * @brief Recreates swapchain with new dimensions
     * @param new_width New width
     * @param new_height New height
     */
    void on_resize(size_t new_width, size_t new_height);

    /**
     * @brief Acquires next swapchain image for rendering
     * @param frame Frame context with semaphore to signal when image is ready
     * @return SwapchainImageData for the acquired image
     */
    FrameRenderingContext prepare_frame(const FrameContext& frame);

    Reference<RenderTarget> get_current_render_target(bool non_linear = true);

    /**
     * @brief Presents rendered image to surface
     * @param frame Frame context with semaphore to wait on before presenting
     */
    void present(const FrameContext& frame);

    /** @brief Gets number of swapchain images */
    [[nodiscard]] size_t get_image_count() const { return images_data.size(); }

    /** @brief Gets swapchain width */
    [[nodiscard]] size_t get_width() const { return width; }

    /** @brief Gets swapchain height */
    [[nodiscard]] size_t get_height() const { return height; }

    /** @brief Gets linear swapchain color format */
    [[nodiscard]] const vk::Format& get_linear_color_format() const { return linear_color_format; }

    /** @brief Gets non linear swapchain color format */
    [[nodiscard]] const vk::Format& get_non_linear_color_format() const { return non_linear_color_format; }

    /** @brief Gets swapchain color space */
    [[nodiscard]] vk::ColorSpaceKHR get_color_space() const { return color_space; }

    [[nodiscard]] VulkanContext& get_context() const { return context; }

    /**
     * @brief Sets vsync enabled/disabled
     * @param new_vsync Whether vsync is enabled
     */
    void set_vsync(const bool new_vsync) { vsync = new_vsync; }

private:
    size_t acquire_next_image(const FrameContext& frame);

    void find_image_format_and_color_space();
    void init_frame_resources();

    void clean_frame(const FrameContext& frame);

private:
    VulkanContext& context;
    bool vsync = false;

    Reference<VulkanSurface> surface;
    vk::raii::SwapchainKHR swapchain = nullptr;

    size_t width = 0, height = 0;
    vk::Format linear_color_format{};
    vk::Format non_linear_color_format{};
    vk::ColorSpaceKHR color_space{};

    std::vector<vk::Image> swap_chain_images;
    std::vector<SwapchainImageData> images_data;

    std::vector<FrameResources> frame_resources;

    // Index of the current swapchain image. can be different from the frame index
    size_t current_image = 0;
    // Index of the frame we are currently working on, up to max frames in flight
    size_t current_frame = 0;

    size_t frames_in_flight = 0;
};
} // portal
