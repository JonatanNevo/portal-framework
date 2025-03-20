//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "common.h"

#include "portal/core/log.h"

namespace portal::vulkan
{
bool is_dynamic_buffer_descriptor_type(const vk::DescriptorType descriptor_type)
{
    return descriptor_type == vk::DescriptorType::eStorageBufferDynamic || descriptor_type == vk::DescriptorType::eUniformBufferDynamic;
}

bool is_buffer_descriptor_type(const vk::DescriptorType descriptor_type)
{
    return descriptor_type == vk::DescriptorType::eStorageBuffer || descriptor_type == vk::DescriptorType::eUniformBuffer ||
        is_dynamic_buffer_descriptor_type(descriptor_type);
}

vk::Format get_suitable_depth_format(
    const vk::PhysicalDevice physical_device,
    const bool depth_only,
    const std::vector<vk::Format>& depth_format_priority_list
)
{
    auto depth_format = vk::Format::eUndefined;
    for (const auto& format : depth_format_priority_list)
    {
        if (depth_only && !is_depth_only_format(format))
            continue;

        auto properties = physical_device.getFormatProperties(format);

        if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        {
            depth_format = format;
            break;
        }
    }

    if (depth_format != vk::Format::eUndefined)
    {
        LOG_CORE_INFO_TAG("Vulkan", "Depth format selected: {}", vk::to_string(depth_format));
        return depth_format;
    }
    throw std::runtime_error("Failed to find a suitable depth format");
}

bool is_depth_only_format(const vk::Format format)
{
    return format == vk::Format::eD16Unorm || format == vk::Format::eD32Sfloat;
}

bool is_depth_stencil_format(const vk::Format format)
{
    return format == vk::Format::eD16UnormS8Uint || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

bool is_depth_format(const vk::Format format)
{
    return is_depth_only_format(format) || is_depth_stencil_format(format);
}
}
