//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once

#include "portal/application/vulkan/base/buffer_pool.h"
#include "portal/application/vulkan/common.h"
#include "portal/application/vulkan/resources/hashing.h"
#include "portal/application/vulkan/command_buffer.h"
#include "portal/application/vulkan/fence_pool.h"
#include "portal/application/vulkan/semaphore_pool.h"


namespace portal::vulkan
{
class DescriptorSet;
class DescriptorPool;
class Queue;
class Device;
class RenderTarget;
}

namespace portal::vulkan::rendering
{

enum BufferAllocationStrategy
{
    OneAllocationPerBuffer,
    MultipleAllocationsPerBuffer
};

enum DescriptorManagementStrategy
{
    StoreInCache,
    CreateDirectly
};

/**
 * @brief RenderFrame is a container for per-frame data, including BufferPool objects,
 * synchronization primitives (semaphores, fences) and the swapchain RenderTarget.
 *
 * When creating a RenderTarget, we need to provide images that will be used as attachments
 * within a RenderPass. The RenderFrame is responsible for creating a RenderTarget using
 * RenderTarget::CreateFunc. A custom RenderTarget::CreateFunc can be provided if a different
 * render target is required.
 *
 * A RenderFrame cannot be destroyed individually since frames are managed by the RenderContext,
 * the whole context must be destroyed. This is because each RenderFrame holds Vulkan objects
 * such as the swapchain image.
 */
class RenderFrame
{
public:
    /**
     * @brief Block size of a buffer pool in kilobytes
     */
    static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

    // A map of the supported usages to a multiplier for the BUFFER_POOL_BLOCK_SIZE
    const std::unordered_map<vk::BufferUsageFlags, uint32_t> supported_usage_map = {
        {vk::BufferUsageFlagBits::eUniformBuffer, 1},
        {vk::BufferUsageFlagBits::eStorageBuffer, 2},
        // x2 the size of BUFFER_POOL_BLOCK_SIZE since SSBOs are normally much larger than other types of buffers
        {vk::BufferUsageFlagBits::eVertexBuffer, 1},
        {vk::BufferUsageFlagBits::eIndexBuffer, 1}
    };

    RenderFrame(Device& device, std::unique_ptr<RenderTarget>&& render_target, size_t thread_count = 1);


    RenderFrame(const RenderFrame&) = delete;
    RenderFrame(RenderFrame&&) = delete;
    RenderFrame& operator=(const RenderFrame&) = delete;
    RenderFrame& operator=(RenderFrame&&) = delete;

    void reset();

    Device& get_device();

    [[nodiscard]] const FencePool& get_fence_pool() const;
    vk::Fence request_fence();

    [[nodiscard]] const SemaphorePool& get_semaphore_pool() const;
    vk::Semaphore request_semaphore();
    vk::Semaphore request_semaphore_with_ownership();
    void release_owned_semaphore(vk::Semaphore semaphore);

    /**
     * @brief Called when the swapchain changes
     * @param render_target A new render target with updated images
     */
    void update_render_target(std::unique_ptr<RenderTarget>&& render_target);
    RenderTarget& get_render_target();
    [[nodiscard]] const RenderTarget& get_render_target() const;

    /**
     * @brief Requests a command buffer to the command pool of the active frame
     *        A frame should be active at the moment of requesting it
     * @param queue The queue command buffers will be submitted on
     * @param reset_mode Indicate how the command buffer will be used, may trigger a
     *        pool re-creation to set necessary flags
     * @param level Command buffer level, either primary or secondary
     * @param thread_index Selects the thread's command pool used to manage the buffer
     * @return A command buffer related to the current active frame
     */
    CommandBuffer& request_command_buffer(
        const Queue& queue,
        CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool,
        vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary,
        size_t thread_index = 0
    );

    [[nodiscard]] vk::DescriptorSet request_descriptor_set(
        const DescriptorSetLayout& descriptor_set_layout,
        const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
        const BindingMap<vk::DescriptorImageInfo>& image_infos,
        bool update_after_bind,
        size_t thread_index = 0
    ) const;

    void clear_descriptors() const;

    /**
     * @brief Sets a new buffer allocation strategy
     * @param new_strategy The new buffer allocation strategy
     */
    void set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy);

    /**
     * @brief Sets a new descriptor set management strategy
     * @param new_strategy The new descriptor set management strategy
     */
    void set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy);


    /**
     * @param usage Usage of the buffer
     * @param size Amount of memory required
     * @param thread_index Index of the buffer pool to be used by the current thread
     * @return The requested allocation, it may be empty
     */
    BufferAllocation allocate_buffer(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index = 0);

    /**
     * @brief Updates all the descriptor sets in the current frame at a specific thread index
     */
    void update_descriptor_sets(size_t thread_index = 0) const;

private:
    Device& device;
    /**
     * @brief Retrieve the frame's command pool(s)
     * @param queue The queue command buffers will be submitted on
     * @param reset_mode Indicate how the command buffers will be reset after execution,
     *        may trigger a pool re-creation to set necessary flags
     * @return The frame's command pool(s)
     */
    std::vector<std::unique_ptr<CommandPool>>& get_command_pools(const Queue& queue, CommandBuffer::ResetMode reset_mode);
    /// Commands pools associated to the frame
    std::map<uint32_t, std::vector<std::unique_ptr<CommandPool>>> command_pools;
    /// Descriptor pools for the frame
    std::vector<std::unique_ptr<std::unordered_map<std::size_t, DescriptorPool>>> descriptor_pools;
    /// Descriptor sets for the frame
    std::vector<std::unique_ptr<std::unordered_map<std::size_t, DescriptorSet>>> descriptor_sets;
    FencePool fence_pool;
    SemaphorePool semaphore_pool;
    size_t thread_count;
    std::unique_ptr<RenderTarget> swapchain_render_target;
    BufferAllocationStrategy buffer_allocation_strategy{BufferAllocationStrategy::MultipleAllocationsPerBuffer};
    DescriptorManagementStrategy descriptor_management_strategy{DescriptorManagementStrategy::StoreInCache};
    std::map<vk::BufferUsageFlags, std::vector<std::pair<BufferPool, BufferBlock*>>> buffer_pools;
    static std::vector<uint32_t> collect_bindings_to_update(
        const DescriptorSetLayout& descriptor_set_layout,
        const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
        const BindingMap<vk::DescriptorImageInfo>& image_infos
    );
};
} // portal
