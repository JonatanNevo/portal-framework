//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once

#include <set>

#include "portal/application/vulkan/command_buffer.h"
#include "portal/application/vulkan/image.h"
#include "portal/application/vulkan/render_target.h"
#include "portal/application/vulkan/swapchain.h"

namespace portal
{
class Window;
}

namespace portal::vulkan
{
class Queue;
class Device;
class Swapchain;
}

namespace portal::vulkan::rendering
{
class RenderFrame;
/**
 * @brief RenderContext acts as a frame manager for the sample, with a lifetime that is the
 * same as that of the Application itself. It acts as a container for RenderFrame objects,
 * swapping between them (begin_frame, end_frame) and forwarding requests for Vulkan resources
 * to the active frame. Note that it's guaranteed that there is always an active frame.
 * More than one frame can be in-flight in the GPU, thus the need for per-frame resources.
 *
 * It requires a Device to be valid on creation, and will take control of a given Swapchain.
 *
 * For normal rendering (using a swapchain), the RenderContext can be created by passing in a
 * swapchain. A RenderFrame will then be created for each Swapchain image.
 *
 * For offscreen rendering (no swapchain), the RenderContext can be given a valid Device, and
 * a width and height. A single RenderFrame will then be created.
 */
class RenderContext
{
public:
    static vk::Format DEFAULT_VK_FORMAT;


    /**
     * @brief Constructor
     * @param device A valid device
     * @param surface A surface, VK_NULL_HANDLE if in offscreen mode
     * @param window The window where the surface was created
     * @param present_mode Requests to set the present mode of the swapchain
     * @param present_mode_priority_list The order in which the swapchain prioritizes selecting its present mode
     * @param surface_format_priority_list The order in which the swapchain prioritizes selecting its surface format
     */
    RenderContext(
        Device& device,
        vk::SurfaceKHR surface,
        const Window& window,
        vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo,
        const std::vector<vk::PresentModeKHR>& present_mode_priority_list = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
        const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list = {
            {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
            {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
        }
    );
    virtual ~RenderContext() = default;

    RenderContext(const RenderContext&) = delete;
    RenderContext(RenderContext&&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
    RenderContext& operator=(RenderContext&&) = delete;

    /**
     * @brief Prepares the RenderFrames for rendering
     * @param thread_count The number of threads in the application, necessary to allocate this many resource pools for each RenderFrame
     * @param create_render_target_func A function delegate, used to create a RenderTarget
     */
    void prepare(
        size_t thread_count = 1,
        std::function<std::unique_ptr<RenderTarget>(Image&&)> create_render_target_func = RenderTarget::DEFAULT_CREATE_FUNC
    );

    /**
     * @brief Recreates the RenderFrames, called after every update
     */
    void recreate();

    /**
     * @brief Updates the swapchains extent, if a swapchain exists
     * @param extent The width and height of the new swapchain images
     */
    void update_swapchain(const vk::Extent2D& extent);

    /**
     * @brief Updates the swapchains image count, if a swapchain exists
     * @param image_count The amount of images in the new swapchain
     */
    void update_swapchain(const uint32_t image_count);

    /**
     * @brief Updates the swapchains image usage, if a swapchain exists
     * @param image_usage_flags The usage flags the new swapchain images will have
     */
    void update_swapchain(const std::set<vk::ImageUsageFlagBits>& image_usage_flags);

    /**
     * @brief Updates the swapchains extent and surface transform, if a swapchain exists
     * @param extent The width and height of the new swapchain images
     * @param transform The surface transform flags
     */
    void update_swapchain(const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform);

    /**
     * @brief Updates the swapchain's compression settings, if a swapchain exists
     * @param compression The compression to use for swapchain images (default, fixed-rate, none)
     * @param compression_fixed_rate The rate to use, if compression is fixed-rate
     */
    void update_swapchain(const vk::ImageCompressionFlagsEXT compression, const vk::ImageCompressionFixedRateFlagsEXT compression_fixed_rate);

    /**
     * @returns True if a valid swapchain exists in the RenderContext
     */
    bool has_swapchain() const;

    /**
     * @brief Recreates the swapchain
     */
    void recreate_swapchain();

    /**
     * @brief Prepares the next available frame for rendering
     * @param reset_mode How to reset the command buffer
     * @returns A valid command buffer to record commands to be submitted
     * Also ensures that there is an active frame if there is no existing active frame already
     */
    CommandBuffer& begin(CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool);

    /**
     * @brief Submits the command buffer to the right queue
     * @param command_buffer A command buffer containing recorded commands
     */
    void submit(CommandBuffer& command_buffer);

    /**
     * @brief Submits multiple command buffers to the right queue
     * @param command_buffers Command buffers containing recorded commands
     */
    void submit(const std::vector<CommandBuffer*>& command_buffers);

    /**
     * @brief begin_frame
     */
    void begin_frame();

    vk::Semaphore submit(
        const Queue& queue,
        const std::vector<CommandBuffer*>& command_buffers,
        vk::Semaphore wait_semaphore,
        vk::PipelineStageFlags wait_pipeline_stage
    );

    /**
     * @brief Submits a command buffer related to a frame to a queue
     */
    void submit(const Queue& queue, const std::vector<CommandBuffer*>& command_buffers) const;

    /**
     * @brief Waits a frame to finish its rendering
     */
    virtual void wait_frame();

    void end_frame(vk::Semaphore semaphore);

    /**
     * @brief An error should be raised if the frame is not active.
     *        A frame is active after @ref begin_frame has been called.
     * @return The current active frame
     */
    [[nodiscard]] RenderFrame& get_active_frame() const;

    /**
     * @brief An error should be raised if the frame is not active.
     *        A frame is active after @ref begin_frame has been called.
     * @return The current active frame index
     */
    uint32_t get_active_frame_index();

    /**
     * @brief An error should be raised if a frame is active.
     *        A frame is active after @ref begin_frame has been called.
     * @return The previous frame
     */
    [[nodiscard]] RenderFrame& get_last_rendered_frame() const;

    [[nodiscard]] vk::Semaphore request_semaphore() const;
    [[nodiscard]] vk::Semaphore request_semaphore_with_ownership() const;
    void release_owned_semaphore(vk::Semaphore semaphore) const;

    [[nodiscard]] Device& get_device() const;

    /**
     * @brief Returns the format that the RenderTargets are created with within the RenderContext
     */
    [[nodiscard]] vk::Format get_format() const;
    [[nodiscard]] const Swapchain& get_swapchain() const;
    [[nodiscard]] const vk::Extent2D& get_surface_extent() const;
    [[nodiscard]] uint32_t get_active_frame_index() const;
    std::vector<std::unique_ptr<RenderFrame>>& get_render_frames();

    /**
     * @brief Handles surface changes, only applicable if the render_context makes use of a swapchain
     */
    virtual bool handle_surface_changes(bool force_update = false);

    /**
     * @brief Returns the WSI acquire semaphore. Only to be used in very special circumstances.
     * @return The WSI acquire semaphore.
     */
    vk::Semaphore consume_acquired_semaphore();

protected:
    vk::Extent2D surface_extent;

private:
    Device& device;
    const Window& window;
    /// If swapchain exists, then this will be a present supported queue, else a graphics queue
    const Queue& queue;
    std::unique_ptr<Swapchain> swapchain;
    SwapchainProperties swapchain_properties;
    std::vector<std::unique_ptr<RenderFrame>> frames;
    vk::Semaphore acquired_semaphore;
    bool prepared{false};
    /// Current active frame index
    uint32_t active_frame_index{0};
    /// Whether a frame is active or not
    bool frame_active{false};
    std::function<std::unique_ptr<RenderTarget>(Image&&)> create_render_target_func = RenderTarget::DEFAULT_CREATE_FUNC;
    vk::SurfaceTransformFlagBitsKHR pre_transform{VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR};
    size_t thread_count{1};
};
} // portal
