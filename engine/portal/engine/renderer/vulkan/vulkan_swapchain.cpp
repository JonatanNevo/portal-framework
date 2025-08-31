//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_swapchain.h"

#include <ranges>

#include <vulkan/vulkan_raii.hpp>

#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/vulkan_render_target.h"

namespace portal::renderer::vulkan
{

static auto logger = Log::get_logger("Vulkan");

vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes, bool vsync)
{
    // The eFifo mode must always be present as per spec
    // This mode waits for the vertical blank ("v-sync")
    auto present_mode = vk::PresentModeKHR::eFifo;

    // If v-sync is not requested, try to find a mailbox mode
    // It's the lowest latency non-tearing present mode available
    if (!vsync)
    {
        for (const auto& mode : available_present_modes)
        {
            if (mode == vk::PresentModeKHR::eMailbox)
            {
                present_mode = mode;
                break;
            }

            if (present_mode == vk::PresentModeKHR::eImmediate)
                present_mode = mode;
        }
    }

    // FIFO is guaranteed to be supported
    return present_mode;
}


void VulkanSwapchain::init(const Ref<VulkanContext>& vulkan_context, GLFWwindow* window)
{
    context = vulkan_context;

    auto physical_device = context->get_physical_device();
    auto device = context->get_device();

    VkSurfaceKHR surface_handle;
    if (glfwCreateWindowSurface(*context->get_instance(), window, nullptr, &surface_handle) != VK_SUCCESS)
    {
        LOGGER_ERROR("Failed to create window surface!");
        //TODO: exit?
        return;
    }
    surface = vk::raii::SurfaceKHR(context->get_instance(), surface_handle);

    // TODO: move queue creation to device somehow?
    const auto queue_family_properties = physical_device->get_handle().getQueueFamilyProperties();

    // Iterate over each queue to learn whether it supports presenting:
    // Find a queue with present support
    // Will be used to present the swap chain images to the windowing system
    std::vector supports_present(queue_family_properties.size(), false);
    for (size_t i = 0; i < queue_family_properties.size(); i++)
    {
        supports_present[i] = physical_device->get_handle().getSurfaceSupportKHR(i, *surface);
    }


    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    size_t graphics_queue_index = std::numeric_limits<size_t>::max();
    size_t present_queue_index = std::numeric_limits<size_t>::max();

    size_t i = 0;
    for (auto& prop : queue_family_properties)
    {
        if ((prop.queueFlags & vk::QueueFlagBits::eGraphics) != 0)
        {
            if (graphics_queue_index == std::numeric_limits<size_t>::max())
                graphics_queue_index = i;

            if (supports_present[i])
            {
                graphics_queue_index = i;
                present_queue_index = i;
                break;
            }
        }
        ++i;
    }

    if (present_queue_index == std::numeric_limits<size_t>::max())
    {
        // If there's no queue that supports both present and graphics
        // try to find a separate present queue
        i = 0;
        for (auto& prop : queue_family_properties)
        {
            if (supports_present[i])
            {
                present_queue_index = i;
                break;
            }
            ++i;
        }
    }

    PORTAL_ASSERT(graphics_queue_index != std::numeric_limits<size_t>::max(), "Failed to find a suitable graphics queue!");
    PORTAL_ASSERT(present_queue_index != std::numeric_limits<size_t>::max(), "Failed to find a suitable present queue!");

    present_queue_family_index = present_queue_index;
    present_queue = device->get_queue(present_queue_index);
    find_image_format_and_color_space();
}


void VulkanSwapchain::create(size_t* request_width, size_t* request_height, const bool new_vsync)
{
    vsync = new_vsync;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Swapchain
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    auto device = context->get_device();
    auto physical_device = context->get_physical_device()->get_handle();

    auto old_swapchain = std::move(swapchain);

    const auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
    const auto present_modes = physical_device.getSurfacePresentModesKHR(*surface);

    vk::Extent2D swapchain_extent = {};
    if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        swapchain_extent = surface_capabilities.currentExtent;
        *request_width = surface_capabilities.currentExtent.width;
        *request_height = surface_capabilities.currentExtent.height;
    }
    else
    {
        swapchain_extent.width = std::clamp<uint32_t>(
            *request_width,
            surface_capabilities.minImageExtent.width,
            surface_capabilities.maxImageExtent.width
            );
        swapchain_extent.height = std::clamp<uint32_t>(
            *request_height,
            surface_capabilities.minImageExtent.height,
            surface_capabilities.maxImageExtent.height
            );
    }

    width = swapchain_extent.width;
    height = swapchain_extent.height;

    if (width == 0 || height == 0)
        return;

    auto present_mode = choose_present_mode(present_modes, vsync);

    const auto min_image_count = std::min(std::max(3u, surface_capabilities.minImageCount), surface_capabilities.maxImageCount);

    // Find the transformation of the surface
    vk::SurfaceTransformFlagBitsKHR pre_transform;
    if (surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        pre_transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    else
        pre_transform = surface_capabilities.currentTransform;

    // Find a supported composite alpha format (not all devices support alpha opaque)
    // By simply select the first composite alpha format available
    auto composite_alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    constexpr std::array composite_alpha_flags = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
        vk::CompositeAlphaFlagBitsKHR::eInherit
    };

    for (const auto& alpha : composite_alpha_flags)
    {
        if (surface_capabilities.supportedCompositeAlpha & alpha)
        {
            composite_alpha = alpha;
            break;
        }
    }

    vk::SwapchainCreateInfoKHR swap_chain_create_info = {
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = surface,
        .minImageCount = min_image_count,
        .imageFormat = color_format,
        .imageColorSpace = color_space,
        .imageExtent = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = pre_transform,
        .compositeAlpha = composite_alpha,
        .presentMode = present_mode,
        .clipped = true,
        .oldSwapchain = old_swapchain
    };

    if (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst)
        swap_chain_create_info.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
    if (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc)
        swap_chain_create_info.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;

    auto&& new_swapchain = device->get_handle().createSwapchainKHR(swap_chain_create_info);
    swapchain = std::move(new_swapchain);
    device->set_debug_name(swapchain, "main swapchain"); // TODO: include window specifier to the name?

    images_data.clear();

    swap_chain_images = swapchain.getImages();
    images_data.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Image view
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        vk::ImageViewCreateInfo view_info{
            .viewType = vk::ImageViewType::e2D,
            .format = color_format,
            .image = swap_chain_images[i],
            .components = {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        auto& [image, image_view, command_pool, command_buffer] = images_data[i];
        image = swap_chain_images[i];
        image_view = device->get_handle().createImageView(view_info);
        device->set_debug_name(image_view, fmt::format("swapchain_image_view_{}", i).c_str());

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Command buffer
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        const vk::CommandPoolCreateInfo pool_info{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = static_cast<uint32_t>(present_queue_family_index),
        };
        command_pool = device->get_handle().createCommandPool(pool_info);
        device->set_debug_name(command_pool, fmt::format("swapchain_command_pool_{}", i).c_str());

        const vk::CommandBufferAllocateInfo alloc_info{
            .commandPool = command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        command_buffer = std::move(device->get_handle().allocateCommandBuffers(alloc_info).front());
        device->set_debug_name(command_buffer, fmt::format("swapchain_command_buffer_{}", i).c_str());
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Frame data
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TODO: have different amount of frames in flight based on some config?
    frames_in_flight = swap_chain_images.size();

    frame_data.clear();
    frame_data.resize(frames_in_flight);

    // create synchronization structures
    // one fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to synchronize rendering with swapchain
    // we want the fence to start signaled so we can wait on it on the first frame
    size_t frame_index = 0;
    for (auto& data : frame_data)
    {
        data.image_available_semaphore = device->get_handle().createSemaphore({});
        device->set_debug_name(data.image_available_semaphore, fmt::format("swapchain_image_available_semaphore_{}", frame_index).c_str());

        data.render_finished_semaphore = device->get_handle().createSemaphore({});
        device->set_debug_name(data.render_finished_semaphore, fmt::format("swapchain_render_finished_semaphore_{}", frame_index).c_str());

        data.wait_fence = device->get_handle().createFence(
            {
                .flags = vk::FenceCreateFlagBits::eSignaled
            }
            );
        device->set_debug_name(data.wait_fence, fmt::format("swapchain_wait_fence_{}", frame_index).c_str());
    }

    // TODO: create render target object?
    Ref<VulkanRenderTarget>::create();
}

void VulkanSwapchain::destroy()
{
    context->get_device()->wait_idle();

    images_data.clear();
    frame_data.clear();

    swapchain = nullptr;

    context->get_device()->wait_idle();
}

void VulkanSwapchain::on_resize(size_t new_width, size_t new_height)
{
    LOGGER_INFO("Resizing swapchain to {}x{}", new_width, new_height);
    context->get_device()->wait_idle();
    create(&new_width, &new_height, vsync);
    context->get_device()->wait_idle();
}

void VulkanSwapchain::begin_frame()
{
    PORTAL_PROF_ZONE();

    current_frame = (current_frame + 1) % frames_in_flight;

    auto& frame = frame_data[current_frame];
    frame.deletion_queue.flush();

    const auto current_image_index = acquire_next_image();
    images_data[current_image_index].command_pool.reset();
}

void VulkanSwapchain::present()
{
    PORTAL_PROF_ZONE();

    const auto& frame = frame_data[current_frame];
    const auto& image = images_data[current_image];

    // TODO: check vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::PipelineStageFlags wait_destination_stage_mask = vk::PipelineStageFlagBits::eAllCommands;

    const vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*frame.image_available_semaphore,
        .pWaitDstStageMask = &wait_destination_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*image.command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*frame.render_finished_semaphore,
    };

    context->get_device()->get_handle().resetFences({frame.wait_fence});

    // TODO: sync device queues?
    context->get_device()->get_graphics_queue().submit(submit_info, frame.wait_fence);

    // Present the current buffer to the swap chain
    // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
    // This ensures that the image is not presented to the windowing system until all commands have been submitted

    {
        PORTAL_PROF_ZONE("VulkanSwapchain::present - present");

        const vk::PresentInfoKHR present_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*frame.render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*swapchain,
            .pImageIndices = reinterpret_cast<uint32_t*>(&current_image)
        };
        try
        {
            const auto result = present_queue.presentKHR(present_info);
            if (result != vk::Result::eSuccess)
            {
                if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
                {
                    on_resize(width, height);
                }
                else
                {
                    LOGGER_ERROR("Error while presenting to the present queue!");
                    //TODO: exit?
                }
            }
        }
        catch (vk::OutOfDateKHRError&)
        {
            on_resize(width, height);
        }
    }
}

size_t VulkanSwapchain::acquire_next_image()
{
    const auto& frame = frame_data[current_frame];
    // Make sure the frame we're requesting has finished rendering (from previous iterations)
    {
        PORTAL_PROF_ZONE("VulkanSwapchain::acquire_next_image - wait for fences");
        context->get_device()->wait_for_fences(std::span{&*frame.wait_fence, 1}, true);
    }

    auto [result, index] = swapchain.acquireNextImage(
        std::numeric_limits<uint64_t>::max(),
        frame.image_available_semaphore,
        nullptr
        );

    if (result != vk::Result::eSuccess)
    {
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
        {
            on_resize(width, height);
            auto [new_result, new_index] = swapchain.acquireNextImage(
                std::numeric_limits<uint64_t>::max(),
                frame.image_available_semaphore,
                nullptr
                );
            if (new_result != vk::Result::eSuccess)
            {
                LOGGER_ERROR("Failed to acquire swapchain image!");
                // TODO: exit?
                return 0;
            }
            index = new_index;
        }
    }

    return index;
}

void VulkanSwapchain::find_image_format_and_color_space()
{
    const auto physical_device = context->get_physical_device()->get_handle();

    auto surface_formats = physical_device.getSurfaceFormatsKHR(*surface);

    // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
    // there is no preferred format, so we assume eB8G8R8A8Unorm
    if (surface_formats.size() == 1 && surface_formats[0].format == vk::Format::eUndefined)
    {
        color_format = vk::Format::eB8G8R8A8Unorm;
        color_space = surface_formats[0].colorSpace;
        return;
    }

    // iterate over the list of available surface format and
    // check for the presence of eB8G8R8A8Srgb
    bool found_srgb = false;
    for (auto&& [format, color_scape] : surface_formats)
    {
        if (format == vk::Format::eB8G8R8A8Srgb && color_space == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            color_format = vk::Format::eB8G8R8A8Srgb;
            color_space = color_scape;
            found_srgb = true;
            break;
        }
    }

    // in case eB8G8R8A8Srgb is not available,
    // we select the first available color format
    if (!found_srgb)
    {
        color_format = surface_formats[0].format;
        color_space = surface_formats[0].colorSpace;
    }
}
} // portal
