//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <deque>
#include <vulkan/vulkan_raii.hpp>


namespace portal::renderer::vulkan
{
class VulkanDevice;
class AllocatedBuffer;
}

namespace portal::renderer::vulkan
{
/**
 * @struct DescriptorWriter
 * @brief Helper for batching descriptor writes
 *
 * Accumulates vk::WriteDescriptorSet operations and applies them in a single update.
 */
struct DescriptorWriter
{
public:
    /**
     * @brief Queues image descriptor write
     * @param binding Binding index
     * @param image_view Vulkan image view
     * @param sampler Vulkan sampler
     * @param layout Image layout
     * @param type Descriptor type
     */
    void write_image(
        uint32_t binding,
        const vk::raii::ImageView& image_view,
        const vk::raii::Sampler& sampler,
        vk::ImageLayout layout,
        vk::DescriptorType type
    );

    /**
     * @brief Queues buffer descriptor write
     * @param binding Binding index
     * @param buffer Allocated buffer
     * @param size Buffer size
     * @param offset Buffer offset
     * @param type Descriptor type
     */
    void write_buffer(uint32_t binding, renderer::vulkan::AllocatedBuffer& buffer, size_t size, size_t offset, vk::DescriptorType type);

    /** @brief Clears all queued writes */
    void clear();

    /**
     * @brief Applies all writes to descriptor set
     * @param device Vulkan device
     * @param set Descriptor set to update
     */
    void update_set(const renderer::vulkan::VulkanDevice& device, const vk::raii::DescriptorSet& set);

    std::deque<vk::DescriptorImageInfo> image_infos;
    std::deque<vk::DescriptorBufferInfo> buffer_infos;
    std::vector<vk::WriteDescriptorSet> writes;
};
}
