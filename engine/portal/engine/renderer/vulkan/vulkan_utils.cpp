//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_utils.h"

#include <GLFW/glfw3.h>

#include "portal/core/log.h"

namespace portal::renderer::vulkan
{
static auto logger = Log::get_logger("Renderer");

vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats)
{
    for (auto& [format, color_space] : available_formats)
    {
        if (format == vk::Format::eB8G8R8A8Srgb && color_space == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return {format, color_space};
        }
    }
    return available_formats[0];
}

vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes)
{
    for (const auto& present_mode : available_present_modes)
    {
        if (present_mode == vk::PresentModeKHR::eMailbox)
            return present_mode;
    }
    // FIFO is guaranteed to be supported
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D choose_extent(uint32_t width, uint32_t height, const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}


uint32_t rate_device_suitability(const vk::raii::PhysicalDevice& device)
{
    uint32_t score = 0;
    const auto properties = device.getProperties();
    const auto features = device.getFeatures();
    const auto queue_families = device.getQueueFamilyProperties();
    //
    // if (properties.apiVersion < vk::ApiVersion14)
    // {
    //     uint32_t major = VK_API_VERSION_MAJOR(properties.apiVersion);
    //     uint32_t minor = VK_API_VERSION_MINOR(properties.apiVersion);
    //     uint32_t patch = VK_API_VERSION_PATCH(properties.apiVersion);
    //     LOGGER_TRACE("Candidate: {} is not supported, API version {}.{}.{} < 1.4", properties.deviceName.data(), major, minor, patch);
    //     return 0;
    // }


    if (std::ranges::find_if(
        queue_families,
        [](const auto& prop)
        {
            return (prop.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
        }
    ) == queue_families.end())
    {
        LOGGER_TRACE("Candidate: {} does not support graphics queue", properties.deviceName.data());
        return 0;
    }

    auto extensions = device.enumerateDeviceExtensionProperties();\
    for (const auto& extension : g_device_extensions)
    {
        if (std::ranges::find_if(extensions, [extension](auto const& ext) { return strcmp(ext.extensionName, extension) == 0; }) == extensions.end())
        {
            LOGGER_TRACE("Candidate: {} does not support extension {}", extension, extension);
            return 0;
        }
    }

    if (!features.samplerAnisotropy)
    {
        LOGGER_TRACE("Candidate: {} does not support sampler anisotropy", properties.deviceName.data());
        return 0;
    }

    // Discrete GPUs have a significant performance advantage
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
    {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += properties.limits.maxImageDimension2D;

    LOGGER_DEBUG("Available Device: {}", properties.deviceName.data());
    return score;
}

uint32_t find_queue_families(const vk::raii::PhysicalDevice& device, vk::QueueFlagBits queue_type)
{
    auto families = device.getQueueFamilyProperties();
    // get the first index into queueFamilyProperties which supports graphics
    const auto graphics_queue_family_prop =
        std::ranges::find_if(
            families,
            [queue_type](const auto& prop)
            {
                return (prop.queueFlags & queue_type) != static_cast<vk::QueueFlags>(0);
            }
        );

    return static_cast<uint32_t>(std::distance(families.begin(), graphics_queue_family_prop));
}

vk::SampleCountFlagBits get_max_usable_sample_count(vk::raii::PhysicalDevice& physical_device)
{
    const auto device_properties = physical_device.getProperties();

    const auto counts = device_properties.limits.framebufferColorSampleCounts & device_properties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }
    return vk::SampleCountFlagBits::e1;
}

std::vector<const char*> get_required_extensions(const vk::raii::Context& context, bool enable_validation_layers)
{
    // Get the required instance extensions from GLFW.
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    // Check if the required GLFW extensions are supported by the Vulkan implementation.
    auto extension_properties = context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfw_extension_count; ++i)
    {
        if (std::ranges::none_of(
            extension_properties,
            [extension=glfw_extensions[i]](const auto& property)
            {
                return strcmp(property.extensionName, extension) == 0;
            }
        ))
        {
            LOG_DEBUG_TAG("Renderer", "Required GLFW extension not supported: {}", glfw_extensions[i]);
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfw_extensions[i]));
        }
    }

    std::vector<const char*> extensions = {glfw_extensions, glfw_extensions + glfw_extension_count};
#ifdef PORTAL_PLATFORM_MACOS
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif
    if (enable_validation_layers)
        extensions.push_back(vk::EXTDebugUtilsExtensionName);


#ifdef PORTAL_DEBUG
    {
        const auto extension_props = context.enumerateInstanceExtensionProperties();
        LOG_TRACE_TAG("Renderer", "Available instance extensions:");
        for (const auto& [extensionName, specVersion] : extension_props)
        {
            std::string extension_name = extensionName;
            auto has_extension = std::ranges::find(extensions, extension_name) != extensions.end() ? "x" : " ";
            LOG_TRACE_TAG("Renderer", "  {} {}", has_extension, extension_name);
        }
    }
    LOG_TRACE_TAG("Renderer", "");
#endif
    return extensions;
}

std::vector<const char*> get_required_validation_layers(const vk::raii::Context& context, bool enable_validation_layers)
{
    // Get required validation layers
    std::vector<const char*> required_layers = {};
    if (enable_validation_layers)
        required_layers.assign(g_validation_layers.begin(), g_validation_layers.end());

    // Check if the required layers are supported by the Vulkan implementation.
    const auto layer_properties = context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(
        required_layers,
        [&layer_properties](const auto& required_layer)
        {
            return std::ranges::none_of(
                layer_properties,
                [required_layer](const auto& layer_property)
                {
                    return strcmp(layer_property.layerName, required_layer) == 0;
                }
            );
        }
    ))
    {
        LOG_DEBUG_TAG("Renderer", "One or more required layers are not supported!");
        throw std::runtime_error("One or more required layers are not supported!");
    }


#ifdef PORTAL_DEBUG
    {
        const auto layers_prop = context.enumerateInstanceLayerProperties();
        LOG_TRACE_TAG("Renderer", "Available instance layers:");
        for (const auto& layer : layers_prop)
        {
            std::string layer_name = layer.layerName;
            auto has_layer = std::ranges::find(required_layers, layer_name) != required_layers.end() ? "v" : " ";
            LOG_TRACE_TAG("Renderer", "  {} {}", has_layer, layer_name);
        }
    }
    LOG_TRACE_TAG("Renderer", "");
#endif
    return required_layers;
}


void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    const uint32_t mip_level,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout
)
{
    vk::PipelineStageFlags2 source_stage;
    vk::PipelineStageFlags2 destination_stage;
    vk::AccessFlags2 src_access_mask;
    vk::AccessFlags2 dst_access_mask;

    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
    {
        src_access_mask = {};
        dst_access_mask = vk::AccessFlagBits2::eTransferWrite;

        source_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits2::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        src_access_mask = vk::AccessFlagBits2::eTransferWrite;
        dst_access_mask = vk::AccessFlagBits2::eShaderRead;

        source_stage = vk::PipelineStageFlagBits2::eTransfer;
        destination_stage = vk::PipelineStageFlagBits2::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    transition_image_layout(
        command_buffer,
        image,
        mip_level,
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        source_stage,
        destination_stage,
        vk::ImageAspectFlagBits::eColor
    );
}

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    const uint32_t mip_level,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask,
    const vk::ImageAspectFlags aspect_mask
)
{
    const vk::ImageSubresourceRange subresource_range{
        .aspectMask = aspect_mask,
        .baseMipLevel = 0,
        .levelCount = mip_level,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers
    };

    transition_image_layout(
        command_buffer,
        image,
        subresource_range,
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        src_stage_mask,
        dst_stage_mask
    );
}

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    const vk::ImageSubresourceRange& subresource,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask
)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresource
    };

    const vk::DependencyInfo dependency_info = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    command_buffer.pipelineBarrier2(dependency_info);
}

void copy_image_to_image(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& source,
    const vk::Image& dest,
    const vk::Extent2D src_size,
    const vk::Extent2D dst_size
)
{
    // Calculate aspect ratios
    const float src_aspect = static_cast<float>(src_size.width) / static_cast<float>(src_size.height);
    const float dst_aspect = static_cast<float>(dst_size.width) / static_cast<float>(dst_size.height);

    // Calculate scaled destination rectangle maintaining aspect ratio
    auto scaled_width = static_cast<int32_t>(dst_size.width);
    auto scaled_height = static_cast<int32_t>(dst_size.height);
    int32_t offset_x = 0;
    int32_t offset_y = 0;

    if (src_aspect > dst_aspect)
    {
        // Source is wider - add letterboxing (black bars on top/bottom)
        scaled_height = static_cast<int32_t>(static_cast<float>(dst_size.width) / src_aspect);
        offset_y = (static_cast<int32_t>(dst_size.height) - scaled_height) / 2;
    }
    else if (src_aspect < dst_aspect)
    {
        // Source is taller - add pillarboxing (black bars on left/right)
        scaled_width = static_cast<int32_t>(static_cast<float>(dst_size.height) * src_aspect);
        offset_x = (static_cast<int32_t>(dst_size.width) - scaled_width) / 2;
    }

    vk::ImageBlit2 blit_region{
        .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .srcOffsets = std::array{vk::Offset3D{}, vk::Offset3D{static_cast<int32_t>(src_size.width), static_cast<int32_t>(src_size.height), 1}},
        .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .dstOffsets = std::array{vk::Offset3D{offset_x, offset_y, 0}, vk::Offset3D{offset_x + scaled_width, offset_y + scaled_height, 1}}
    };

    const vk::BlitImageInfo2 blit_info = {
        .srcImage = source,
        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
        .dstImage = dest,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = &blit_region,
        .filter = vk::Filter::eLinear
    };

    command_buffer.blitImage2(blit_info);
}
}
