//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "render_frame.h"

#include <ranges>
#include <set>

#include "portal/application/vulkan/command_pool.h"
#include "portal/application/vulkan/descriptor_pool.h"
#include "portal/application/vulkan/descriptor_set.h"
#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/fence_pool.h"
#include "portal/application/vulkan/semaphore_pool.h"
#include "portal/application/vulkan/resources/hashing.h"

namespace portal::vulkan::rendering
{
RenderFrame::RenderFrame(Device& device, std::unique_ptr<RenderTarget>&& render_target, size_t thread_count)
    : device(device),
      fence_pool(device),
      semaphore_pool(device),
      swapchain_render_target(std::move(render_target)),
      thread_count(thread_count)
{
    for (auto& [usage, scale] : supported_usage_map)
    {
        auto [buffer_pools_it, inserted] = buffer_pools.emplace(usage, std::vector<std::pair<BufferPool, BufferBlock*>>{});
        if (!inserted)
            throw std::runtime_error("failed to add buffer pool");

        for (size_t i = 0; i < thread_count; ++i)
            buffer_pools_it->second.push_back(std::make_pair(BufferPool(device, BUFFER_POOL_BLOCK_SIZE * 1024 * scale, usage), nullptr));
    }

    for (size_t i = 0; i < thread_count; ++i)
    {
        descriptor_pools.push_back(std::make_unique<std::unordered_map<std::size_t, DescriptorPool>>());
        descriptor_sets.push_back(std::make_unique<std::unordered_map<std::size_t, DescriptorSet>>());
    }
}

void RenderFrame::reset()
{
    const auto res = fence_pool.wait();
    if (res != vk::Result::eSuccess)
        throw std::runtime_error("failed to wait for fences");

    fence_pool.reset();

    for (const auto& command_pools_per_queue : command_pools | std::views::values)
    {
        for (const auto& command_pool : command_pools_per_queue)
        {
            command_pool->reset_pool();
        }
    }

    for (auto& buffer_pools_per_usage : buffer_pools | std::views::values)
    {
        for (auto& [buffer_pool, buffer_block] : buffer_pools_per_usage)
        {
            buffer_pool.reset();
            buffer_block = nullptr;
        }
    }

    semaphore_pool.reset();

    if (descriptor_management_strategy == CreateDirectly)
    {
        clear_descriptors();
    }
}

Device& RenderFrame::get_device()
{
    return device;
}

const FencePool& RenderFrame::get_fence_pool() const
{
    return fence_pool;
}

vk::Fence RenderFrame::request_fence()
{
    return fence_pool.request_fence();
}

const SemaphorePool& RenderFrame::get_semaphore_pool() const
{
    return semaphore_pool;
}

vk::Semaphore RenderFrame::request_semaphore()
{
    return semaphore_pool.request_semaphore();
}

vk::Semaphore RenderFrame::request_semaphore_with_ownership()
{
    return semaphore_pool.request_semaphore_with_ownership();
}

void RenderFrame::release_owned_semaphore(const vk::Semaphore semaphore)
{
    semaphore_pool.release_owned_semaphore(semaphore);
}

void RenderFrame::update_render_target(std::unique_ptr<RenderTarget>&& render_target)
{
    swapchain_render_target = std::move(render_target);
}

RenderTarget& RenderFrame::get_render_target()
{
    return *swapchain_render_target;
}

const RenderTarget& RenderFrame::get_render_target() const
{
    return *swapchain_render_target;
}

CommandBuffer& RenderFrame::request_command_buffer(
    const Queue& queue,
    CommandBuffer::ResetMode reset_mode,
    vk::CommandBufferLevel level,
    size_t thread_index
)
{
    PORTAL_CORE_ASSERT(thread_index < thread_count, "thread index out of range");

    auto& command_pools = get_command_pools(queue, reset_mode);
    const auto command_pool_it = std::ranges::find_if(
        command_pools,
        [&thread_index](auto& cmd_pool) { return cmd_pool->get_thread_index() == thread_index; }
    );

    if (command_pool_it == command_pools.end())
        throw std::runtime_error("failed to find command pool");

    return (*command_pool_it)->request_command_buffer(level);
}

vk::DescriptorSet RenderFrame::request_descriptor_set(
    const DescriptorSetLayout& descriptor_set_layout,
    const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
    const BindingMap<vk::DescriptorImageInfo>& image_infos,
    const bool update_after_bind,
    const size_t thread_index
) const
{
    PORTAL_CORE_ASSERT(thread_index < thread_count, "thread index out of range");
    PORTAL_CORE_ASSERT(thread_index < descriptor_pools.size(), "descriptor pool index out of range");

    auto& descriptor_pool = request_resource(device, *descriptor_pools[thread_index], descriptor_set_layout);
    if (descriptor_management_strategy == DescriptorManagementStrategy::StoreInCache)
    {
        // The bindings we want to update before binding, if empty we update all bindings
        std::vector<uint32_t> bindings_to_update;
        // If update after bind is enabled, we store the binding index of each binding that need to be updated before being bound
        if (update_after_bind)
            bindings_to_update = collect_bindings_to_update(descriptor_set_layout, buffer_infos, image_infos);

        PORTAL_CORE_ASSERT(thread_index < descriptor_sets.size(), "descriptor set index out of range");
        auto& descriptor_set = request_resource(
            device,
            *descriptor_sets[thread_index],
            descriptor_set_layout,
            descriptor_pool,
            buffer_infos,
            image_infos
        );
        descriptor_set.update(bindings_to_update);
        return descriptor_set.get_handle();
    }

    // Request a descriptor pool, allocate a descriptor set, write buffer and image data to it
    DescriptorSet descriptor_set(device, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
    descriptor_set.apply_writes();
    return descriptor_set.get_handle();
}

void RenderFrame::clear_descriptors() const
{
    for (const auto& desc_sets_per_thread : descriptor_sets)
    {
        desc_sets_per_thread->clear();
    }

    for (const auto& desc_pools_per_thread : descriptor_pools)
    {
        for (auto& desc_pool : *desc_pools_per_thread | std::views::values)
        {
            desc_pool.reset();
        }
    }
}

void RenderFrame::set_buffer_allocation_strategy(const BufferAllocationStrategy new_strategy)
{
    buffer_allocation_strategy = new_strategy;
}

void RenderFrame::set_descriptor_management_strategy(const DescriptorManagementStrategy new_strategy)
{
    descriptor_management_strategy = new_strategy;
}

BufferAllocation RenderFrame::allocate_buffer(vk::BufferUsageFlags usage, vk::DeviceSize size, size_t thread_index)
{
    PORTAL_CORE_ASSERT(thread_index < thread_count, "thread index out of range");

    // Find a pool for this usage
    auto buffer_pool_it = buffer_pools.find(usage);
    if (buffer_pool_it == buffer_pools.end())
    {
        LOG_CORE_ERROR_TAG("Vulkan", "No buffer pool for buffer usage " + vk::to_string(usage));
        return BufferAllocation{};
    }

    PORTAL_CORE_ASSERT(thread_index < buffer_pool_it->second.size(), "buffer pool index out of range");
    auto& buffer_pool = buffer_pool_it->second[thread_index].first;
    auto& buffer_block = buffer_pool_it->second[thread_index].second;

    const bool want_minimal_block = buffer_allocation_strategy == OneAllocationPerBuffer;

    if (want_minimal_block || !buffer_block || !buffer_block->can_allocate(size))
    {
        // If we are creating a buffer for each allocation of there is no block associated with the pool or the current block is too small
        // for this allocation, request a new buffer block
        buffer_block = &buffer_pool.request_buffer_block(size, want_minimal_block);
    }

    return buffer_block->allocate(size);
}

void RenderFrame::update_descriptor_sets(const size_t thread_index) const
{
    PORTAL_CORE_ASSERT(thread_index < thread_count, "thread index out of range");

    auto& thread_descriptor_sets = *descriptor_sets[thread_index];
    for (auto& descriptor_set : thread_descriptor_sets | std::views::values)
    {
        descriptor_set.update();
    }
}

std::vector<std::unique_ptr<CommandPool>>& RenderFrame::get_command_pools(const Queue& queue, CommandBuffer::ResetMode reset_mode)
{
    auto command_pool_it = command_pools.find(queue.get_family_index());
    if (command_pool_it != command_pools.end())
    {
        if (command_pool_it->second[0]->get_reset_mode() != reset_mode)
        {
            device.get_handle().waitIdle();
            // Delete pools
            command_pools.erase(command_pool_it);
        }
        else
            return command_pool_it->second;
    }

    bool inserted = false;
    std::tie(command_pool_it, inserted) = command_pools.emplace(queue.get_family_index(), std::vector<std::unique_ptr<CommandPool>>{});
    if (!inserted)
        throw std::runtime_error("Failed to insert command pool");

    for (size_t i = 0; i < thread_count; i++)
    {
        command_pool_it->second.push_back(std::make_unique<CommandPool>(device, queue.get_family_index(), this, i, reset_mode));
    }

    return command_pool_it->second;
}

std::vector<uint32_t> RenderFrame::collect_bindings_to_update(
    const DescriptorSetLayout& descriptor_set_layout,
    const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
    const BindingMap<vk::DescriptorImageInfo>& image_infos
)
{
    std::set<uint32_t> bindings_to_update;
    auto aggregate_binding_to_update = [&bindings_to_update, &descriptor_set_layout](const auto& infos_map)
    {
        for (const auto& [binding_index, ignored] : infos_map)
        {
            if (!(descriptor_set_layout.get_layout_binding_flag(binding_index) & vk::DescriptorBindingFlagBits::eUpdateAfterBind))
            {
                bindings_to_update.insert(binding_index);
            }
        }
    };

    aggregate_binding_to_update(buffer_infos);
    aggregate_binding_to_update(image_infos);

    return {bindings_to_update.begin(), bindings_to_update.end()};
}
} // portal
