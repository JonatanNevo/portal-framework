//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_device.h"

#include <volk.h>

#include "vulkan_utils.h"
#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/renderer/vulkan/pipeline_builder.h"
#include "device/vulkan_physical_device.h"
#include "llvm/ADT/SmallVector.h"
#include "portal/engine/renderer/vulkan/vulkan_instance.h"

namespace portal::renderer::vulkan
{
static auto logger = Log::get_logger("Vulkan");

VulkanDevice::VulkanDevice(const VulkanPhysicalDevice& physical_device, const VulkanPhysicalDevice::Features& device_features)
    : physical_device(physical_device)
{
    std::vector<const char*> device_extensions;
    device_extensions.insert(device_extensions.end(), REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());

    if (physical_device.is_extension_supported(vk::NVDeviceDiagnosticCheckpointsExtensionName))
        device_extensions.push_back(vk::NVDeviceDiagnosticCheckpointsExtensionName);

    if (physical_device.is_extension_supported(vk::NVDeviceDiagnosticsConfigExtensionName))
        device_extensions.push_back(vk::NVDeviceDiagnosticsConfigExtensionName);

    if (physical_device.is_extension_supported(vk::EXTDebugMarkerExtensionName) && physical_device.is_extension_supported("VK_EXT_debug_report"))
    {
        device_extensions.push_back(vk::EXTDebugMarkerExtensionName);
        device_extensions.push_back("VK_EXT_debug_report"); // Required for moltenvk
        debug_marker_enabled = true;
    }

#if !defined(PORTAL_DIST)
    // Try to enable calibrated timestamps extension (prefer KHR, fallback to EXT)
    if (physical_device.is_extension_supported(vk::KHRCalibratedTimestampsExtensionName))
    {
        device_extensions.push_back(vk::KHRCalibratedTimestampsExtensionName);
        LOGGER_DEBUG("Using VK_KHR_calibrated_timestamps extension");
    }
    else if (physical_device.is_extension_supported(vk::EXTCalibratedTimestampsExtensionName))
    {
        device_extensions.push_back(vk::EXTCalibratedTimestampsExtensionName);
        LOGGER_DEBUG("Using VK_EXT_calibrated_timestamps extension");
    }
    else
    {
        LOGGER_WARN("Calibrated timestamps extension not available");
    }
#endif


    // Get queue family indices for the requested queue family types
    // Note that the indices may overlap depending on the implementation
    constexpr vk::QueueFlags requested_queue_types = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
    const auto [graphics, compute, transfer] = physical_device.get_queue_family_indices(requested_queue_types);

    static constexpr float default_queue_priority = 0.0f;

    const std::vector<vk::QueueFamilyProperties>& queue_family_properties = physical_device.get_queue_family_properties();
    llvm::SmallVector<vk::DeviceQueueCreateInfo> queue_create_infos;

    queue_create_infos.reserve(queue_family_properties.size());

    // Graphics queue
    if (requested_queue_types & vk::QueueFlagBits::eGraphics)
    {
        queue_create_infos.emplace_back(
            vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = static_cast<uint32_t>(graphics),
                .queueCount = 1,
                .pQueuePriorities = &default_queue_priority,
            }
        );
    }

    if (requested_queue_types & vk::QueueFlagBits::eCompute)
    {
        if (compute != graphics)
        {
            queue_create_infos.emplace_back(
                vk::DeviceQueueCreateInfo{
                    .queueFamilyIndex = static_cast<uint32_t>(compute),
                    .queueCount = 1,
                    .pQueuePriorities = &default_queue_priority,
                }
            );
        }
    }

    if (requested_queue_types & vk::QueueFlagBits::eTransfer)
    {
        if (transfer != graphics && transfer != compute)
        {
            queue_create_infos.emplace_back(
                vk::DeviceQueueCreateInfo{
                    .queueFamilyIndex = static_cast<uint32_t>(transfer),
                    .queueCount = 1,
                    .pQueuePriorities = &default_queue_priority,
                }
            );
        }
    }

    const vk::DeviceCreateInfo create_info = {
        .pNext = &device_features.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
    };

    device = physical_device.get_handle().createDevice(create_info);
    volkLoadDevice(*device);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

    queues.emplace(QueueType::Graphics, VulkanQueue{*this, static_cast<size_t>(graphics), queue_family_properties[graphics], 0, false});
    set_debug_name(queues.at(QueueType::Graphics).get_handle(), "graphics queue");

    if (compute != -1)
    {
        queues.emplace(QueueType::Compute, VulkanQueue{*this, static_cast<size_t>(compute), queue_family_properties[compute], 0, false});
        set_debug_name(queues.at(QueueType::Compute).get_handle(), "graphics queue");
    }

    if (transfer != -1)
    {
        queues.emplace(QueueType::Transfer, VulkanQueue{*this, static_cast<size_t>(transfer), queue_family_properties[transfer], 0, false});
        set_debug_name(queues.at(QueueType::Transfer).get_handle(), "graphics queue");
    }

    initialize_immediate_commands();
    pipeline_cache = device.createPipelineCache({});
}

void VulkanDevice::add_present_queue(Surface& surface)
{
    const auto queue_family_properties = physical_device.get_queue_family_properties();

    // Iterate over each queue to learn whether it supports presenting:
    // Find a queue with present support
    // Will be used to present the swap chain images to the windowing system

    std::vector supports_present(queue_family_properties.size(), false);
    for (uint32_t i = 0; i < queue_family_properties.size(); i++)
    {
        supports_present[i] = physical_device.supports_present(surface, i);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    size_t graphics_queue_index = std::numeric_limits<size_t>::max();
    size_t present_queue_index = std::numeric_limits<size_t>::max();

    size_t i = 0;
    for (auto& prop : queue_family_properties)
    {
        if ((prop.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits{0})
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
        for (size_t index = 0; index < queue_family_properties.size(); ++index)
        {
            if (supports_present[index])
            {
                present_queue_index = index;
                break;
            }
        }
    }

    PORTAL_ASSERT(graphics_queue_index != std::numeric_limits<size_t>::max(), "Failed to find a suitable graphics queue!");
    PORTAL_ASSERT(present_queue_index != std::numeric_limits<size_t>::max(), "Failed to find a suitable present queue!");

    queues.emplace(QueueType::Present, VulkanQueue{*this, present_queue_index, queue_family_properties[present_queue_index], 0, true});
}

AllocatedBuffer VulkanDevice::create_buffer(const BufferBuilder& builder) const
{
    return builder.build(*this);
}

std::shared_ptr<AllocatedBuffer> VulkanDevice::create_buffer_shared(const BufferBuilder& builder) const
{
    return builder.build_shared(*this);
}

ImageAllocation VulkanDevice::create_image(const ImageBuilder& builder) const
{
    return builder.build(*this);
}

vk::ImageView VulkanDevice::create_image_view(const vk::ImageViewCreateInfo& info) const
{
    return (*device).createImageView(info);
}

void VulkanDevice::destory_image_view(vk::ImageView image_view) const
{
    (*device).destroyImageView(image_view);
}

vk::raii::Sampler VulkanDevice::create_sampler(const vk::SamplerCreateInfo& info) const
{
    return device.createSampler(info);
}

vk::raii::DescriptorSetLayout VulkanDevice::create_descriptor_set_layout(portal::renderer::vulkan::DescriptorLayoutBuilder& builder) const
{
    return builder.build(device);
}

vk::raii::PipelineLayout VulkanDevice::create_pipeline_layout(const vk::PipelineLayoutCreateInfo& pipeline_layout_info) const
{
    return device.createPipelineLayout(pipeline_layout_info);
}

vk::raii::ShaderModule VulkanDevice::create_shader_module(const Buffer& code) const
{
    return device.createShaderModule(
        {
            .codeSize = code.size * sizeof(char),
            .pCode = code.as<uint32_t*>()
        }
    );
}

vk::raii::Pipeline VulkanDevice::create_pipeline(PipelineBuilder& builder) const
{
    return builder.build(device, pipeline_cache);
}

void VulkanDevice::immediate_submit(std::function<void(const vk::raii::CommandBuffer&)>&& function) const
{
    device.resetFences({immediate_command_buffer.fence});

    immediate_command_buffer.command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    function(immediate_command_buffer.command_buffer);
    immediate_command_buffer.command_buffer.end();

    vk::CommandBufferSubmitInfo cmd_submit_info{
        .commandBuffer = immediate_command_buffer.command_buffer,
        .deviceMask = 0
    };

    vk::SubmitInfo2 submit_info{
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_submit_info
    };

    get_transfer_queue().submit({submit_info}, immediate_command_buffer.fence);
    wait_for_fences({immediate_command_buffer.fence}, true, std::numeric_limits<uint64_t>::max());

    immediate_command_buffer.command_pool.reset({});
    // No need to reallocate; the existing buffer handle remains valid and is reset by pool reset in RAII
}

void VulkanDevice::wait_for_fences(const vk::ArrayProxy<const vk::Fence> fences, const bool wait_all, const size_t timeout) const
{
    const auto result = get_handle().waitForFences(fences, wait_all, timeout);
    if (result != vk::Result::eSuccess)
        LOGGER_ERROR("Error while waiting for fences: {}", vk::to_string(result));
    //TODO: exit on error?
}

void VulkanDevice::wait_idle() const
{
    return get_handle().waitIdle();
}


const VulkanQueue& VulkanDevice::get_graphics_queue() const
{
    return queues.at(QueueType::Graphics);
}

const VulkanQueue& VulkanDevice::get_compute_queue() const
{
    if (queues.contains(QueueType::Compute))
        return queues.at(QueueType::Compute);
    return get_graphics_queue();
}

const VulkanQueue& VulkanDevice::get_transfer_queue() const
{
    if (queues.contains(QueueType::Transfer))
        return queues.at(QueueType::Transfer);
    return get_graphics_queue();
}

const VulkanQueue& VulkanDevice::get_present_queue() const
{
    if (queues.contains(QueueType::Present))
        return queues.at(QueueType::Present);
    return get_graphics_queue();
}

void VulkanDevice::set_debug_name(const vk::ObjectType type, const uint64_t handle, const char* name) const
{
    if constexpr (ENABLE_VALIDATION_LAYERS)
    {
        get_handle().setDebugUtilsObjectNameEXT(
            vk::DebugUtilsObjectNameInfoEXT{
                .objectType = type,
                .objectHandle = handle,
                .pObjectName = name
            }
        );
    }
}

void VulkanDevice::initialize_immediate_commands()
{
    const vk::CommandPoolCreateInfo pool_info{
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = static_cast<uint32_t>(get_transfer_queue().get_family_index()),
        // TODO: should this be transfer or graphics?
    };

    auto& [pool, buffer, fence] = immediate_command_buffer;
    pool = device.createCommandPool(pool_info);
    set_debug_name(pool, "immediate command pool");

    const vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    buffer = std::move(device.allocateCommandBuffers(alloc_info).front());
    set_debug_name(buffer, "immediate command buffer");

    fence = device.createFence({});
    set_debug_name(fence, "immediate command fence");
}
} // portal
