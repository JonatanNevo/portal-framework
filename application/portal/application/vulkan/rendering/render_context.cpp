//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "render_context.h"

#include "portal/application/vulkan/device.h"
#include "portal/application/window/window.h"
#include "portal/application/vulkan/rendering/render_frame.h"

namespace portal::vulkan::rendering
{
vk::Format RenderContext::DEFAULT_VK_FORMAT = vk::Format::eR8G8B8A8Srgb;

RenderContext::RenderContext(
    Device& device,
    vk::SurfaceKHR surface,
    const Window& window,
    vk::PresentModeKHR present_mode,
    const std::vector<vk::PresentModeKHR>& present_mode_priority_list,
    const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list
): surface_extent{window.get_extent().width, window.get_extent().height},
   device{device},
   window{window},
   queue{device.get_suitable_graphics_queue()},
   swapchain_properties()
{
    if (surface)
    {
        const auto surface_properties = device.get_gpu().get_handle().getSurfaceCapabilitiesKHR(surface);
        if (surface_properties.currentExtent.width == 0xFFFFFFFF)
        {
            swapchain = std::make_unique<Swapchain>(
                device,
                surface,
                present_mode,
                present_mode_priority_list,
                surface_format_priority_list,
                surface_extent
            );
        }
        else
        {
            swapchain = std::make_unique<Swapchain>(device, surface, present_mode, present_mode_priority_list, surface_format_priority_list);
        }
    }
}

void RenderContext::prepare(size_t thread_count, std::function<std::unique_ptr<RenderTarget>(Image&&)> create_render_target_func)
{
    device.get_handle().waitIdle();
    if (swapchain)
    {
        surface_extent = swapchain->get_extent();

        vk::Extent3D extent{surface_extent.width, surface_extent.height, 1};
        for (auto& image_handle : swapchain->get_images())
        {
            auto swapchain_image = Image{device, image_handle, extent, swapchain->get_format(), swapchain->get_usage()};
            auto render_target = create_render_target_func(std::move(swapchain_image));
            frames.emplace_back(std::make_unique<RenderFrame>(device, std::move(render_target), thread_count));
        }
    }
    else
    {
        // Otherwise, create a single RenderFrame
        swapchain = nullptr;
        ImageBuilder builder(vk::Extent3D{surface_extent.width, surface_extent.height, 1});
        builder.with_format(DEFAULT_VK_FORMAT).with_usage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc).
                with_vma_usage(vma::MemoryUsage::eGpuOnly);
        auto color_image = builder.build(device);

        auto render_target = create_render_target_func(std::move(color_image));
        frames.emplace_back(std::make_unique<RenderFrame>(device, std::move(render_target), thread_count));
    }

    this->create_render_target_func = create_render_target_func;
    this->thread_count = thread_count;
    prepared = true;
}

void RenderContext::recreate()
{
    LOG_CORE_INFO_TAG("Vulkan", "Recreating swapchain");
    const vk::Extent2D swapchain_extent = swapchain->get_extent();
    const vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

    auto frame_it = frames.begin();
    for (auto& image_handle : swapchain->get_images())
    {
        Image swapchain_image{device, image_handle, extent, swapchain->get_format(), swapchain->get_usage()};

        auto render_target = create_render_target_func(std::move(swapchain_image));
        if (frame_it != frames.end())
            (*frame_it)->update_render_target(std::move(render_target));
        else
        {
            // Create a new frame if the new swapchain has more images than current frames
            frames.emplace_back(std::make_unique<RenderFrame>(device, std::move(render_target), thread_count));
        }

        ++frame_it;
    }

    device.get_resource_cache().clear_framebuffers();
}

void RenderContext::update_swapchain(const vk::Extent2D& extent)
{
    if (!swapchain)
    {
        LOG_CORE_WARN_TAG("Vulkan", "Can't update the swapchains extent. No swapchain, offscreen rendering detected, skipping.");
        return;
    }

    device.get_resource_cache().clear_framebuffers();

    swapchain = std::make_unique<Swapchain>(*swapchain, extent);

    recreate();
}

void RenderContext::update_swapchain(const uint32_t image_count)
{
    if (!swapchain)
    {
        LOG_CORE_WARN_TAG("Vulkan", "Can't update the swapchains extent. No swapchain, offscreen rendering detected, skipping.");
        return;
    }

    device.get_resource_cache().clear_framebuffers();

    device.get_handle().waitIdle();

    swapchain = std::make_unique<Swapchain>(*swapchain, image_count);

    recreate();
}

void RenderContext::update_swapchain(const std::set<vk::ImageUsageFlagBits>& image_usage_flags)
{
    if (!swapchain)
    {
        LOG_CORE_WARN_TAG("Vulkan", "Can't update the swapchains extent. No swapchain, offscreen rendering detected, skipping.");
        return;
    }

    device.get_resource_cache().clear_framebuffers();

    swapchain = std::make_unique<Swapchain>(*swapchain, image_usage_flags);

    recreate();
}

void RenderContext::update_swapchain(const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform)
{
    if (!swapchain)
    {
        LOG_CORE_WARN_TAG("Vulkan", "Can't update the swapchains extent. No swapchain, offscreen rendering detected, skipping.");
        return;
    }


    auto width = extent.width;
    auto height = extent.height;
    if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate90 || transform == vk::SurfaceTransformFlagBitsKHR::eRotate270)
    {
        // Pre-rotation: always use native orientation i.e. if rotated, use width and height of identity transform
        std::swap(width, height);
    }

    swapchain = std::make_unique<Swapchain>(*swapchain, vk::Extent2D{width, height}, transform);

    // Save the preTransform attribute for future rotations
    pre_transform = transform;

    device.get_resource_cache().clear_framebuffers();

    recreate();
}

void RenderContext::update_swapchain(
    const vk::ImageCompressionFlagsEXT compression,
    const vk::ImageCompressionFixedRateFlagsEXT compression_fixed_rate
)
{
    if (!swapchain)
    {
        LOG_CORE_WARN_TAG("Vulkan", "Can't update the swapchains extent. No swapchain, offscreen rendering detected, skipping.");
        return;
    }

    device.get_resource_cache().clear_framebuffers();

    swapchain = std::make_unique<Swapchain>(*swapchain, compression, compression_fixed_rate);

    recreate();
}

bool RenderContext::has_swapchain() const
{
    return swapchain != nullptr;
}

void RenderContext::recreate_swapchain()
{
    device.get_handle().waitIdle();
    device.get_resource_cache().clear_framebuffers();

    const vk::Extent2D swapchain_extent = swapchain->get_extent();
    const vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

    auto frame_it = frames.begin();
    for (auto& image_handle : swapchain->get_images())
    {
        Image swapchain_image{device, image_handle, extent, swapchain->get_format(), swapchain->get_usage()};
        auto render_target = create_render_target_func(std::move(swapchain_image));
        (*frame_it)->update_render_target(std::move(render_target));
        ++frame_it;
    }
}

CommandBuffer& RenderContext::begin(CommandBuffer::ResetMode reset_mode)
{
    PORTAL_CORE_ASSERT(prepared, "RenderContext not prepared, call prepare() before using it");

    if (!frame_active)
        begin_frame();

    if (!acquired_semaphore)
        throw std::runtime_error("Couldn't begin frame");

    const auto queue = device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
    return get_active_frame().request_command_buffer(queue, reset_mode);
}

void RenderContext::submit(CommandBuffer& command_buffer)
{
    submit({&command_buffer});
}

void RenderContext::submit(const std::vector<CommandBuffer*>& command_buffers)
{
    PORTAL_CORE_ASSERT(frame_active, "RenderContext not active, call begin() before submitting commands");

    vk::Semaphore render_semaphore;

    if (swapchain)
    {
        PORTAL_CORE_ASSERT(acquired_semaphore, "We do not have acquired_semaphore, it was probably consumed?");
        render_semaphore = submit(queue, command_buffers, acquired_semaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
    }
    else
    {
        submit(queue, command_buffers);
    }

    end_frame(render_semaphore);
}

void RenderContext::begin_frame()
{
    // Only handle surface changes if a swapchain exists
    if (swapchain)
        handle_surface_changes();

    PORTAL_CORE_ASSERT(!frame_active, "Frame is still active, please call end_frame");
    auto& prev_frame = *frames[active_frame_index];

    // We will use the acquired semaphore in a different frame context,
    // so we need to hold ownership.
    acquired_semaphore = prev_frame.request_semaphore_with_ownership();

    if (swapchain)
    {
        vk::Result result;
        try
        {
            std::tie(result, active_frame_index) = swapchain->acquire_next_image(acquired_semaphore);
        }
        catch (vk::OutOfDateKHRError& /*err*/)
        {
            result = vk::Result::eErrorOutOfDateKHR;
        }


        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
        {
#if defined(PORTAL_PLATFORM_MACOS)
            // On Apple platforms, force swapchain update on both eSuboptimalKHR and eErrorOutOfDateKHR
            // eSuboptimalKHR may occur on macOS/iOS following changes to swapchain other than extent/size
            bool swapchain_updated = handle_surface_changes(true);
#else
            bool swapchain_updated = handle_surface_changes(result == vk::Result::eErrorOutOfDateKHR);
#endif

            if (swapchain_updated)
            {
                // Need to destroy and reallocate acquired_semaphore since it may have already been signaled
                device.get_handle().destroySemaphore(acquired_semaphore);
                acquired_semaphore = prev_frame.request_semaphore_with_ownership();
                std::tie(result, active_frame_index) = swapchain->acquire_next_image(acquired_semaphore);
            }
        }

        if (result != vk::Result::eSuccess)
        {
            prev_frame.reset();
            return;
        }
    }

    // Now the frame is active again
    frame_active = true;
    // Wait on all resource to be freed from the previous render to this frame
    wait_frame();
}

vk::Semaphore RenderContext::submit(
    const Queue& queue,
    const std::vector<CommandBuffer*>& command_buffers,
    vk::Semaphore wait_semaphore,
    const vk::PipelineStageFlags wait_pipeline_stage
)
{
    std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
    std::ranges::transform(
        command_buffers,
        cmd_buf_handles.begin(),
        [](const CommandBuffer* cmd_buf) { return cmd_buf->get_handle(); }
    );

    RenderFrame& frame = get_active_frame();
    vk::Semaphore signal_semaphore = frame.request_semaphore();
    vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles, signal_semaphore);

    if (wait_semaphore)
    {
        submit_info.setWaitSemaphores(wait_semaphore);
        submit_info.pWaitDstStageMask = &wait_pipeline_stage;
    }

    const vk::Fence fence = frame.request_fence();
    queue.get_handle().submit(submit_info, fence);
    return signal_semaphore;
}

void RenderContext::submit(const Queue& queue, const std::vector<CommandBuffer*>& command_buffers) const
{
    std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
    std::ranges::transform(
        command_buffers,
        cmd_buf_handles.begin(),
        [](const CommandBuffer* cmd_buf) { return cmd_buf->get_handle(); }
    );

    RenderFrame& frame = get_active_frame();
    const vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles);

    const vk::Fence fence = frame.request_fence();
    queue.get_handle().submit(submit_info, fence);
}

void RenderContext::wait_frame()
{
    get_active_frame().reset();
}

void RenderContext::end_frame(vk::Semaphore semaphore)
{
    PORTAL_CORE_ASSERT(frame_active, "Frame is not active, please call begin_frame");

    if (swapchain)
    {
        vk::SwapchainKHR vk_swapchain = swapchain->get_handle();
        vk::PresentInfoKHR present_info(semaphore, vk_swapchain, active_frame_index);

        vk::DisplayPresentInfoKHR disp_present_info;
        if (device.is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) && window.get_display_present_info(
            &disp_present_info,
            surface_extent.width,
            surface_extent.height
        ))
        {
            // Add display present info if supported and wanted
            present_info.pNext = &disp_present_info;
        }


        vk::Result result;
        try
        {
            result = queue.present(present_info);
        }
        catch (vk::OutOfDateKHRError& /*err*/)
        {
            result = vk::Result::eErrorOutOfDateKHR;
        }

        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
        {
            handle_surface_changes();
        }
    }

    // Frame is not active anymore
    if (acquired_semaphore)
    {
        release_owned_semaphore(acquired_semaphore);
        acquired_semaphore = nullptr;
    }
    frame_active = false;
}

RenderFrame& RenderContext::get_active_frame() const
{
    PORTAL_CORE_ASSERT(frame_active, "Frame is not active, please call begin_frame");
    return *frames[active_frame_index];
}

uint32_t RenderContext::get_active_frame_index()
{
    PORTAL_CORE_ASSERT(frame_active, "Frame is not active, please call begin_frame");
    return active_frame_index;
}

RenderFrame& RenderContext::get_last_rendered_frame() const
{
    PORTAL_CORE_ASSERT(!frame_active, "Frame is still active, please call end_frame");
    return *frames[active_frame_index];
}

vk::Semaphore RenderContext::request_semaphore() const
{
    return get_active_frame().request_semaphore();
}

vk::Semaphore RenderContext::request_semaphore_with_ownership() const
{
    return get_active_frame().request_semaphore_with_ownership();
}

void RenderContext::release_owned_semaphore(const vk::Semaphore semaphore) const
{
    get_active_frame().release_owned_semaphore(semaphore);
}

Device& RenderContext::get_device() const
{
    return device;
}

vk::Format RenderContext::get_format() const
{
    return swapchain ? swapchain->get_format() : DEFAULT_VK_FORMAT;
}

const Swapchain& RenderContext::get_swapchain() const
{
    PORTAL_CORE_ASSERT(swapchain, "Swapchain is not valid");
    return *swapchain;
}

const vk::Extent2D& RenderContext::get_surface_extent() const
{
    return surface_extent;
}

uint32_t RenderContext::get_active_frame_index() const
{
    return active_frame_index;
}

std::vector<std::unique_ptr<RenderFrame>>& RenderContext::get_render_frames()
{
    return frames;
}

bool RenderContext::handle_surface_changes(bool force_update)
{
    if (!swapchain)
    {
        LOG_CORE_WARN_TAG("Vulkan", "Can't handle surface changes. No swapchain, offscreen rendering detected, skipping.");
        return false;
    }

    const vk::SurfaceCapabilitiesKHR surface_properties = device.get_gpu().get_handle().getSurfaceCapabilitiesKHR(swapchain->get_surface());
    if (surface_properties.currentExtent.width == 0xFFFFFFFF)
    {
        return false;
    }

    // Only recreate the swapchain if the dimensions have changed;
    // handle_surface_changes() is called on VK_SUBOPTIMAL_KHR,
    // which might not be due to a surface resize
    if (surface_properties.currentExtent.width != surface_extent.width ||
        surface_properties.currentExtent.height != surface_extent.height ||
        force_update)
    {
        // Recreate swapchain
        device.get_handle().waitIdle();
        update_swapchain(surface_properties.currentExtent, pre_transform);
        surface_extent = surface_properties.currentExtent;
        return true;
    }


    return false;
}

vk::Semaphore RenderContext::consume_acquired_semaphore()
{
    return std::exchange(acquired_semaphore, nullptr);
}
} // portal
