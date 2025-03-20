//
// Created by Jonatan Nevo on 02/03/2025.
//

#include "device.h"

#include <ranges>

#include "portal/application/vulkan/command_pool.h"
#include "portal/application/vulkan/base/allocated.h"

namespace portal::vulkan
{
Device::Device(
    PhysicalDevice& gpu,
    vk::SurfaceKHR surface,
    std::unique_ptr<DebugUtils>&& debug_utils,
    std::unordered_map<const char*, bool> requested_extensions
): VulkanResource(nullptr, this), gpu(gpu), debug_utils(std::move(debug_utils)), resource_cache(*this)
{
    LOG_CORE_INFO_TAG("Vulkan", "Selected GPU: {}", gpu.get_properties().deviceName.data());

    const std::vector<vk::QueueFamilyProperties> queue_family_properties = gpu.get_queue_family_properties();
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos(queue_family_properties.size());
    std::vector<std::vector<float>> queue_priorities(queue_family_properties.size());

    for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index)
    {
        const auto& queue_family_property = queue_family_properties[queue_family_index];

        if (gpu.has_high_priority_graphics_queue())
        {
            const uint32_t graphics_queue_family = get_queue_family_index(vk::QueueFlagBits::eGraphics);
            if (graphics_queue_family == queue_family_index)
            {
                queue_priorities[queue_family_index].reserve(queue_family_property.queueCount);
                queue_priorities[queue_family_index].push_back(1.0f);
                for (uint32_t i = 1; i < queue_family_property.queueCount; i++)
                {
                    queue_priorities[queue_family_index].push_back(0.5f);
                }
            }
            else
            {
                queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);
            }
        }
        else
        {
            queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);
        }

        vk::DeviceQueueCreateInfo& queue_create_info = queue_create_infos[queue_family_index];

        queue_create_info.queueFamilyIndex = queue_family_index;
        queue_create_info.queueCount = queue_family_property.queueCount;
        queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
    }

    // Check extensions to enable Vma Dedicated Allocation
    bool can_get_memory_requirements = is_extension_supported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    bool has_dedicated_allocation = is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

    if (can_get_memory_requirements && has_dedicated_allocation)
    {
        enabled_extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
        enabled_extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

        LOG_CORE_INFO_TAG("Vulkan", "Dedicated Allocation enabled");
    }

    // For performance queries, we also use host query reset since queryPool resets cannot live in the same command buffer as beginQuery
    if (is_extension_supported(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME) && is_extension_supported(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME))
    {
        auto perf_counter_features = gpu.get_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>();
        auto host_query_reset_features = gpu.get_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>();

        if (perf_counter_features.performanceCounterQueryPools && host_query_reset_features.hostQueryReset)
        {
            gpu.add_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>().performanceCounterQueryPools = true;
            gpu.add_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>().hostQueryReset = true;
            enabled_extensions.push_back(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);
            enabled_extensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
            LOG_CORE_INFO_TAG("Vulkan", "Performance query enabled");
        }
    }

    // Check that extensions are supported before trying to create the device
    std::vector<const char*> unsupported_extensions{};
    for (const auto& name : requested_extensions | std::views::keys)
    {
        if (is_extension_supported(name))
            enabled_extensions.emplace_back(name);
        else
            unsupported_extensions.emplace_back(name);
    }

    if (!enabled_extensions.empty())
    {
        LOG_CORE_DEBUG_TAG("Vulkan", "Device supports the following requested extensions:");
        for (auto& extension : enabled_extensions)
            LOG_CORE_DEBUG_TAG("Vulkan", "  \t{}", extension);
    }

    if (!unsupported_extensions.empty())
    {
        auto error = false;
        for (auto& extension : unsupported_extensions)
        {
            auto extension_is_optional = requested_extensions[extension];
            if (extension_is_optional)
            {
                LOG_CORE_WARN_TAG("Vulkan", "Optional device extension {} not available, some features may be disabled", extension);
            }
            else
            {
                LOG_CORE_ERROR_TAG("Vulkan", "Required device extension {} not available, cannot run", extension);
                error = true;
            }
        }

        if (error)
            throw std::runtime_error("Extensions not present");
    }

    vk::DeviceCreateInfo create_info({}, queue_create_infos, {}, enabled_extensions, &gpu.get_mutable_requested_features());
    // Latest requested feature will have the pNext's all set up for device creation.
    create_info.pNext = gpu.get_extension_feature_chain();
    set_handle(gpu.get_handle().createDevice(create_info));

    queues.resize(queue_family_properties.size());
    for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index)
    {
        const auto& queue_family_property = queue_family_properties[queue_family_index];

        vk::Bool32 present_supported = gpu.get_handle().getSurfaceSupportKHR(queue_family_index, surface);
        for (uint32_t queue_index = 0U; queue_index < queue_family_property.queueCount; ++queue_index)
        {
            queues[queue_family_index].emplace_back(*this, queue_family_index, queue_family_property, present_supported, queue_index);
        }
    }

    portal::vulkan::allocated::init(*this);

    command_pool = std::make_unique<CommandPool>(
        *this,
        get_queue_by_flags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index()
    );
    fence_pool = std::make_unique<FencePool>(*this);
}

Device::~Device()
{
    resource_cache.clear();

    command_pool.reset();
    fence_pool.reset();

    portal::vulkan::allocated::shutdown();

    if (has_handle())
    {
        get_handle().destroy();
    }
}

const PhysicalDevice& Device::get_gpu() const
{
    return gpu;
}

const DebugUtils& Device::get_debug_utils() const
{
    return *debug_utils;
}

const Queue& Device::get_queue(const uint32_t queue_family_index, const uint32_t queue_index) const
{
    return queues[queue_family_index][queue_index];
}

const Queue& Device::get_queue_by_flags(const vk::QueueFlags queue_flags, const uint32_t queue_index) const
{
    for (const auto& queue : queues)
    {
        const Queue& first_queue = queue[0];

        vk::QueueFlags family_flags = first_queue.get_properties().queueFlags;
        const uint32_t queue_count = first_queue.get_properties().queueCount;
        if (((family_flags & queue_flags) == queue_flags) && queue_index < queue_count)
        {
            return queue[queue_index];
        }
    }

    throw std::runtime_error("Queue not found");
}

const Queue& Device::get_queue_by_present(const uint32_t queue_index) const
{
    for (const auto& queue : queues)
    {
        const Queue& first_queue = queue[0];

        const uint32_t queue_count = first_queue.get_properties().queueCount;
        if (first_queue.support_present() && queue_index < queue_count)
        {
            return queue[queue_index];
        }
    }

    throw std::runtime_error("Queue not found");
}

const Queue& Device::get_suitable_graphics_queue() const
{
    for (const auto& queue : queues)
    {
        const Queue& first_queue = queue[0];
        const uint32_t queue_count = first_queue.get_properties().queueCount;
        if (first_queue.support_present() && 0 < queue_count)
        {
            return queue[0];
        }
    }
    return get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
}

bool Device::is_extension_supported(const std::string& extension) const
{
    return gpu.is_extension_supported(extension);
}

bool Device::is_enabled(const std::string& extension) const
{
    return std::ranges::any_of(
        enabled_extensions,
        [&extension](const auto& enabled_extension)
        {
            return extension == enabled_extension;
        }
    );
}

uint32_t Device::get_queue_family_index(const vk::QueueFlagBits queue_flag) const
{
    const auto& queue_family_properties = gpu.get_queue_family_properties();

    // Dedicated queue for compute
    // Try to find a queue family index that supports compute but not graphics
    if (queue_flag & vk::QueueFlagBits::eCompute)
    {
        for (size_t i = 0; i < queue_family_properties.size(); i++)
        {
            if ((queue_family_properties[i].queueFlags & queue_flag) && !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics))
            {
                return static_cast<uint32_t>(i);
            }
        }
    }

    // For other queue types or if no separate compute queue is present, return the first one to support the requested
    // flags
    for (size_t i = 0; i < queue_family_properties.size(); i++)
    {
        if (queue_family_properties[i].queueFlags & queue_flag)
        {
            return static_cast<uint32_t>(i);
        }
    }

    throw std::runtime_error("Could not find a matching queue family index");
}

CommandPool& Device::get_command_pool() const
{
    return *command_pool;
}

std::pair<vk::Image, vk::DeviceMemory> Device::create_image(
    const vk::Format format,
    vk::Extent2D const& extent,
    const uint32_t mip_levels,
    const vk::ImageUsageFlags usage,
    const vk::MemoryPropertyFlags properties
) const
{
    const vk::Device device = get_handle();

    const vk::ImageCreateInfo image_create_info(
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D(extent, 1),
        mip_levels,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        usage
    );
    vk::Image image = device.createImage(image_create_info);

    const vk::MemoryRequirements memory_requirements = device.getImageMemoryRequirements(image);
    const vk::MemoryAllocateInfo memory_allocation(
        memory_requirements.size,
        get_gpu().get_memory_type(memory_requirements.memoryTypeBits, properties)
    );
    vk::DeviceMemory memory = device.allocateMemory(memory_allocation);

    device.bindImageMemory(image, memory, 0);
    return std::make_pair(image, memory);
}

void Device::copy_buffer(Buffer& src, Buffer& dst, const vk::Queue queue, const vk::BufferCopy* copy_region) const
{
    PORTAL_CORE_ASSERT(dst.get_size() <= src.get_size(), "Destination buffer is larger than source buffer");
    PORTAL_CORE_ASSERT(src.get_handle(), "Source buffer is invalid");
    PORTAL_CORE_ASSERT(dst.get_handle(), "Destination buffer is invalid");

    const vk::CommandBuffer command_buffer = create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

    vk::BufferCopy buffer_copy{};
    if (copy_region)
        buffer_copy = *copy_region;
    else
        buffer_copy.size = src.get_size();
    command_buffer.copyBuffer(src.get_handle(), dst.get_handle(), buffer_copy);
    flush_command_buffer(command_buffer, queue);
}

vk::CommandBuffer Device::create_command_buffer(vk::CommandBufferLevel level, bool begin) const
{
    PORTAL_CORE_ASSERT(command_pool, "Command pool is invalid");
    const vk::CommandBuffer command_buffer = get_handle().allocateCommandBuffers({command_pool->get_handle(), level, 1}).front();

    // If requested, also start recording for the new command buffer
    if (begin)
        command_buffer.begin(vk::CommandBufferBeginInfo());

    return command_buffer;
}

void Device::flush_command_buffer(vk::CommandBuffer command_buffer, const vk::Queue queue, const bool free, vk::Semaphore signalSemaphore) const
{
    if (!command_buffer)
        return;

    command_buffer.end();

    vk::SubmitInfo submit_info({}, {}, command_buffer);
    if (signalSemaphore)
        submit_info.setSignalSemaphores(signalSemaphore);

    // Create fence to ensure that the command buffer has finished executing
    const vk::Fence fence = get_handle().createFence({});

    // Submit to the queue
    queue.submit(submit_info, fence);

    // Wait for the fence to signal that command buffer has finished executing
    const vk::Result result = get_handle().waitForFences(fence, true, DEFAULT_FENCE_TIMEOUT);
    if (result != vk::Result::eSuccess)
    {
        LOG_CORE_ERROR_TAG("Vulkan", "Detected Vulkan error: {}", vk::to_string(result));
        abort();
    }

    get_handle().destroyFence(fence);
    if (command_pool && free)
        get_handle().freeCommandBuffers(command_pool->get_handle(), command_buffer);
}

FencePool& Device::get_fence_pool() const
{
    return *fence_pool;
}

ResourceCache& Device::get_resource_cache()
{
    return resource_cache;
}
} // portal
