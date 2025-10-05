//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_device.h"

#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/renderer/vulkan/pipeline_builder.h"
#include "portal/engine/renderer/vulkan/vulkan_init.h"
#include "portal/engine/renderer/vulkan/vulkan_physical_device.h"

namespace portal::renderer::vulkan
{

static auto logger = Log::get_logger("Vulkan");

VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice>& physical_device, const Features& device_features) : physical_device(physical_device),
    enabled_features(device_features)
{
    std::vector<const char*> device_extensions;
    device_extensions.append_range(DEVICE_EXTENSIONS);

    if (physical_device->is_extension_supported(vk::NVDeviceDiagnosticCheckpointsExtensionName))
        device_extensions.push_back(vk::NVDeviceDiagnosticCheckpointsExtensionName);

    if (physical_device->is_extension_supported(vk::NVDeviceDiagnosticsConfigExtensionName))
        device_extensions.push_back(vk::NVDeviceDiagnosticsConfigExtensionName);

    if (physical_device->is_extension_supported(vk::EXTDebugMarkerExtensionName))
    {
        device_extensions.push_back(vk::EXTDebugMarkerExtensionName);
        debug_marker_enabled = true;
    }

    const vk::DeviceCreateInfo create_info = {
        .pNext = &device_features.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<uint32_t>(physical_device->queue_create_infos.size()),
        .pQueueCreateInfos = physical_device->queue_create_infos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
    };

    device = physical_device->physical_device.createDevice(create_info);

    const auto [graphics, compute, transfer] = physical_device->get_queue_family_indices();

    graphics_queue = get_handle().getQueue(graphics, 0);
    set_debug_name(graphics_queue, "graphics queue");

    if (compute != -1)
    {
        compute_queue = get_handle().getQueue(compute, 0);
        set_debug_name(compute_queue, "compute queue");
    }

    if (transfer != -1)
    {
        transfer_queue = get_handle().getQueue(transfer, 0);
        set_debug_name(transfer_queue, "transfer queue");
    }

    initialize_immediate_commands();
    pipeline_cache = device.createPipelineCache({});
}

AllocatedBuffer VulkanDevice::create_buffer(const BufferBuilder& builder)
{
    return builder.build({this});
}

std::shared_ptr<AllocatedBuffer> VulkanDevice::create_buffer_shared(const BufferBuilder& builder)
{
    return builder.build_shared({this});
}

AllocatedImage VulkanDevice::create_image(const ImageBuilder& builder)
{
    return builder.build({this});
}

vk::raii::ImageView VulkanDevice::create_image_view(const vk::ImageViewCreateInfo& info) const
{
    auto image_view =  device.createImageView(info);
    return image_view;
}

vk::raii::Sampler VulkanDevice::create_sampler(const vk::SamplerCreateInfo& info) const
{
    return device.createSampler(info);
}

vk::raii::DescriptorSetLayout VulkanDevice::create_descriptor_set_layout(portal::vulkan::DescriptorLayoutBuilder& builder) const
{
    return builder.build(device);
}

vk::raii::PipelineLayout VulkanDevice::create_pipeline_layout(const vk::PipelineLayoutCreateInfo& pipeline_layout_info) const
{
    return device.createPipelineLayout(pipeline_layout_info);
}

vk::raii::ShaderModule VulkanDevice::create_shader_module(const Buffer& code) const
{
    return device.createShaderModule({
        .codeSize = code.size * sizeof(char),
        .pCode = code.as<uint32_t*>()
    });
}

vk::raii::Pipeline VulkanDevice::create_pipeline(PipelineBuilder& builder) const
{
    return builder.build(device, pipeline_cache);
}

void VulkanDevice::immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function)
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

    get_transfer_queue().submit2({submit_info}, immediate_command_buffer.fence);
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

vk::Queue VulkanDevice::get_queue(const size_t index, const StringId& name) const
{
    const auto queue = get_handle().getQueue(static_cast<uint32_t>(index), 0);
    set_debug_name(transfer_queue, name);
    return queue;
}

void VulkanDevice::set_debug_name(const vk::ObjectType type, const uint64_t handle, const char* name) const
{

    get_handle().setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT{
            .objectType = type,
            .objectHandle = handle,
            .pObjectName = name
        }
        );
}

void VulkanDevice::initialize_immediate_commands()
{
    const auto indices = physical_device->get_queue_family_indices();
    const vk::CommandPoolCreateInfo pool_info{
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = static_cast<uint32_t>(indices.transfer), // TODO: should this be transfer or graphics?
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
