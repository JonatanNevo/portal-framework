//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_surface.h"

#include <GLFW/glfw3.h>

#include "portal/application/settings.h"
#include "portal/engine/window/glfw_window.h"

namespace portal::renderer::vulkan
{
static auto logger = Log::get_logger("Vulkan");

SurfaceTransform to_surface_transform(const vk::SurfaceTransformFlagsKHR transform)
{
    SurfaceTransform output = SurfaceTransformBits::Emtpy;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        output |= SurfaceTransformBits::Identity;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate90)
        output |= SurfaceTransformBits::Rotate90;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate180)
        output |= SurfaceTransformBits::Rotate180;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate270)
        output |= SurfaceTransformBits::Rotate270;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror)
        output |= SurfaceTransformBits::Mirror;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90)
        output |= SurfaceTransformBits::MirrorRotate90;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180)
        output |= SurfaceTransformBits::MirrorRotate180;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270)
        output |= SurfaceTransformBits::MirrorRotate270;

    if (transform & vk::SurfaceTransformFlagBitsKHR::eInherit)
        output |= SurfaceTransformBits::Inherit;

    return output;
}

VulkanSurface::VulkanSurface(ProjectSettings& settings, const VulkanContext& context, const SurfaceProperties& properties) : Surface(properties)
{
    if (properties.window.has_value())
    {
        // TODO: support surface types that are not only glfw
        auto& glfw_window = dynamic_cast<GlfwWindow&>(properties.window.value().get());

        VkSurfaceKHR surface_handle;
        if (glfwCreateWindowSurface(*context.get_instance(), glfw_window.get_handle(), nullptr, &surface_handle) != VK_SUCCESS)
        {
            LOGGER_ERROR("Failed to create window surface!");
            throw std::runtime_error("Failed to create window surface!");
        }

        surface = vk::raii::SurfaceKHR(context.get_instance(), surface_handle);
    }
    else
    {
        surface = context.get_instance().createHeadlessSurfaceEXT({});
    }

    context.get_device().set_debug_name(surface, properties.debug_name);

    auto vulkan_capabilities = context.get_physical_device().get_handle().getSurfaceCapabilitiesKHR(*surface);

    glm::ivec2 current_extent;
    if (vulkan_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() || vulkan_capabilities.currentExtent.height !=
        std::numeric_limits<uint32_t>::max())
    {
        current_extent = {vulkan_capabilities.currentExtent.width, vulkan_capabilities.currentExtent.height};
    }
    else
    {
        auto [width, height] = properties.window.value().get().get_extent();
        current_extent = {width, height};
    }

    if (vulkan_capabilities.maxImageCount == 0)
    {
        vulkan_capabilities.maxImageCount = std::numeric_limits<uint32_t>::max();
    }

    capabilities = SurfaceCapabilities{
        .min_swapchain_images = static_cast<size_t>(vulkan_capabilities.minImageCount),
        .max_swapchain_images = static_cast<size_t>(vulkan_capabilities.maxImageCount),
        .current_extent = current_extent,
        .min_image_extent = {vulkan_capabilities.minImageExtent.width, vulkan_capabilities.minImageExtent.height},
        .max_image_extent = {vulkan_capabilities.maxImageExtent.width, vulkan_capabilities.maxImageExtent.height},
        .max_image_array_layers = vulkan_capabilities.maxImageArrayLayers,
        .supported_transforms = to_surface_transform(vulkan_capabilities.supportedTransforms),
        .current_transform = to_surface_transform(vulkan_capabilities.currentTransform)
    };

    if (get_min_frames_in_flight() > properties.min_frames_in_flight)
    {
        settings.set_setting("application.frames_in_flight", get_min_frames_in_flight());
    }
}

const SurfaceCapabilities& VulkanSurface::get_capabilities() const
{
    return capabilities;
}

glm::ivec2 VulkanSurface::get_extent() const
{
    return capabilities.current_extent;
}

vk::SurfaceKHR VulkanSurface::get_vulkan_surface() const
{
    return surface;
}

SurfaceType VulkanSurface::get_type() const
{
    // TODO: support headless
    return SurfaceType::Normal;
}

size_t VulkanSurface::get_min_frames_in_flight() const
{
    return std::min(std::max(properties.min_frames_in_flight, capabilities.min_swapchain_images), capabilities.max_swapchain_images);
}
}
