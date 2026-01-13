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
#include "device/vulkan_physical_device.h"
#include "portal/engine/renderer/rendering_context.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "render_target/vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "render_target/vulkan_swapchain_render_target.h"

namespace portal::renderer::vulkan
{
static auto logger = Log::get_logger("Vulkan");

vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes, const bool vsync)
{
    // The eFifo mode must always be present as per properties
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

            if (mode == vk::PresentModeKHR::eImmediate)
                present_mode = mode;
        }
    }

    // FIFO is guaranteed to be supported
    return present_mode;
}

VulkanSwapchain::VulkanSwapchain(const VulkanContext& context, const Reference<Surface>& surface) : context(context),
    surface(reference_cast<VulkanSurface>(surface))
{
    find_image_format_and_color_space();

    auto extent = surface->get_extent();
    // TODO: propagate vsync to swapchain?
    create(reinterpret_cast<uint32_t*>(&extent.x), reinterpret_cast<uint32_t*>(&extent.y), true);
    if (extent != surface->get_extent())
    {
        LOG_WARN("Extent changed during swapchain creation");
    }
}

void VulkanSwapchain::create(uint32_t* request_width, uint32_t* request_height, const bool new_vsync)
{
    vsync = new_vsync;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Swapchain
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    auto& device = context.get_device();
    auto physical_device = context.get_physical_device().get_handle();

    auto old_swapchain = std::move(swapchain);

    const auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface->get_vulkan_surface());
    const auto present_modes = physical_device.getSurfacePresentModesKHR(surface->get_vulkan_surface());

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

    const auto min_image_count = surface->get_min_frames_in_flight();

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
        .surface = surface->get_vulkan_surface(),
        .minImageCount = static_cast<uint32_t>(min_image_count),
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

    auto&& new_swapchain = device.get_handle().createSwapchainKHR(swap_chain_create_info);
    swapchain = std::move(new_swapchain);
    device.set_debug_name(swapchain, "main swapchain"); // TODO: include window specifier to the name?

    images_data.clear();

    swap_chain_images = swapchain.getImages();
    images_data.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Image view
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        vk::ImageViewCreateInfo view_info{
            .image = swap_chain_images[i],
            .viewType = vk::ImageViewType::e2D,
            .format = color_format,
            .components = {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        auto& [image_props, image, image_view, last_used_frame, render_finished_semaphore] = images_data[i];
        image_props = {
            .format = to_format(color_format),
            .usage = ImageUsage::Attachment,
            .transfer = false,
            .width = width,
            .height = height,
            .depth = 1,
            .mips = 1,
            .layers = 1,
            .create_sampler = false,
            .name = STRING_ID(fmt::format("swapchain_image_{}", i)),
        };
        image = swap_chain_images[i];
        image_view = device.get_handle().createImageView(view_info);
        render_finished_semaphore = device.get_handle().createSemaphore({});

        device.set_debug_name(image_view, fmt::format("swapchain_image_view_{}", i).c_str());
        device.set_debug_name(render_finished_semaphore, fmt::format("swapchain_render_finished_semaphore_{}", i).c_str());
    }
}

void VulkanSwapchain::destroy()
{
    context.get_device().wait_idle();

    images_data.clear();
    swapchain = nullptr;

    context.get_device().wait_idle();
}

void VulkanSwapchain::on_resize(size_t new_width, size_t new_height)
{
    LOGGER_INFO("Resizing swapchain to {}x{}", new_width, new_height);
    context.get_device().wait_idle();
    create(reinterpret_cast<uint32_t*>(&new_width), reinterpret_cast<uint32_t*>(&new_height), vsync);
    context.get_device().wait_idle();
}

SwapchainImageData& VulkanSwapchain::begin_frame(const FrameContext& frame)
{
    PORTAL_PROF_ZONE();

    current_image = acquire_next_image(frame);
    auto& image_data = images_data[current_image];
    image_data.last_used_frame = frame.frame_index;

    return images_data[current_image];
}

void VulkanSwapchain::present(const FrameContext& frame)
{
    PORTAL_PROF_ZONE();
    constexpr vk::PipelineStageFlags2 wait_destination_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

    auto* rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

    const vk::SemaphoreSubmitInfo wait_semaphore_info{
        .semaphore = *rendering_context->resources.image_available_semaphore,
        .value = 0,
        // Assuming binary semaphores are used (value is ignored/defaulted)
        .stageMask = wait_destination_stage_mask,
        .deviceIndex = 0,
    };

    const vk::SemaphoreSubmitInfo signal_semaphore_info{
        .semaphore = *images_data[current_image].render_finished_semaphore,
        .value = 0,
        // Assuming binary semaphores are used
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands,
        // Signal when all commands are done
        .deviceIndex = 0,
    };

    const vk::CommandBufferSubmitInfo command_buffer_info{
        .commandBuffer = *rendering_context->command_buffer,
        .deviceMask = 0
    };

    const vk::SubmitInfo2 submit_info{
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &wait_semaphore_info,

        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &command_buffer_info,

        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signal_semaphore_info,
    };

    context.get_device().get_handle().resetFences({rendering_context->resources.wait_fence});

    // TODO: sync device queues?
    context.get_device().get_graphics_queue().submit(submit_info, rendering_context->resources.wait_fence);

    // Present the current buffer to the swap chain
    // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
    // This ensures that the image is not presented to the windowing system until all commands have been submitted

    {
        PORTAL_PROF_ZONE("VulkanSwapchain::present - present");

        const vk::PresentInfoKHR present_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*images_data[current_image].render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*swapchain,
            .pImageIndices = reinterpret_cast<uint32_t*>(&current_image)
        };
        try
        {
            const auto result = context.get_device().get_present_queue().present(present_info);
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

Reference<RenderTarget> VulkanSwapchain::make_render_target()
{
    RenderTargetProperties properties{
        .width = get_width(),
        .height = get_height(),
        .attachments = {
            // TODO: Is this static? would this change based on settings? Do I need to recreate the render target on swapchain reset?
            .attachment_images = {
                // Present Image
                {
                    .format = vulkan::to_format(get_color_format()),
                    .blend = false
                },
                // Depth Image
                {
                    .format = ImageFormat::Depth_32Float,
                    .blend = true,
                    .blend_mode = BlendMode::Additive
                }
            },
            .blend = true,
        },
        .transfer = true,
        .name = STRING_ID("geometry-render-target"),
    };
    return make_reference<VulkanSwapchainRenderTarget>(properties, *this);
}

size_t VulkanSwapchain::acquire_next_image(const FrameContext& frame)
{
    auto* rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

    // Make sure the frame we're requesting has finished rendering (from previous iterations)
    {
        PORTAL_PROF_ZONE("VulkanSwapchain::acquire_next_image - wait for fences");
        context.get_device().wait_for_fences(std::span{&*rendering_context->resources.wait_fence, 1}, true);
    }

    auto [result, index] = swapchain.acquireNextImage(
        std::numeric_limits<uint64_t>::max(),
        rendering_context->resources.image_available_semaphore,
        nullptr
    );

    if (result != vk::Result::eSuccess)
    {
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
        {
            on_resize(width, height);
            auto [new_result, new_index] = swapchain.acquireNextImage(
                std::numeric_limits<uint64_t>::max(),
                rendering_context->resources.image_available_semaphore,
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
    const auto physical_device = context.get_physical_device().get_handle();

    auto surface_formats = physical_device.getSurfaceFormatsKHR(surface->get_vulkan_surface());

    // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
    // there is no preferred format, so we assume eR8G8B8A8Unorm
    if (surface_formats.size() == 1 && surface_formats[0].format == vk::Format::eUndefined)
    {
        color_format = vk::Format::eR8G8B8A8Unorm;
        color_space = surface_formats[0].colorSpace;
        return;
    }

    // iterate over the list of available surface format and
    // check for the presence of eB8G8R8A8Srgb
    bool found_srgb = false;
    for (auto&& [format, color_scape] : surface_formats)
    {
        if (format == vk::Format::eR8G8B8A8Srgb && color_space == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            color_format = vk::Format::eR8G8B8A8Srgb;
            color_space = color_scape;
            found_srgb = true;
            break;
        }
    }

    // in case eR8G8B8A8Srgb is not available,
    // we select the first available color format
    if (!found_srgb)
    {
        color_format = surface_formats[0].format;
        color_space = surface_formats[0].colorSpace;
    }
}
} // portal
