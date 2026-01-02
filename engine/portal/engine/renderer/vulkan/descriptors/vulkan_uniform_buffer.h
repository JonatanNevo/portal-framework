//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/reference.h"
#include "portal/engine/renderer/descriptors/uniform_buffer.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"

namespace portal::renderer::vulkan
{
/**
 * @class VulkanUniformBuffer
 * @brief Vulkan uniform buffer with VMA allocation
 *
 * CPU-visible uniform buffer for shader constants. Maintains local storage copy.
 */
class VulkanUniformBuffer final : public UniformBuffer
{
public:
    /**
     * @brief Constructs Vulkan uniform buffer
     * @param size Buffer size in bytes
     * @param device Vulkan device
     */
    VulkanUniformBuffer(size_t size, const VulkanDevice& device);
    ~VulkanUniformBuffer() override;

    /**
     * @brief Uploads data to GPU
     * @param data CPU buffer
     * @param offset Byte offset
     */
    void set_data(Buffer data, size_t offset) override;

    /** @brief Gets CPU buffer (const) */
    const Buffer& get_data() const override;

    /** @brief Gets descriptor buffer info for binding */
    const vk::DescriptorBufferInfo& get_descriptor_buffer_info() const;

private:
    /** @brief Releases GPU resources */
    void release();

    /** @brief Allocates GPU buffer */
    void init();

private:
    AllocatedBuffer buffer;
    size_t size;

    Buffer local_storage = nullptr;
    vk::DescriptorBufferInfo descriptor_buffer_info;

    const VulkanDevice& device;
};

/**
 * @class VulkanUniformBufferSet
 * @brief Collection of Vulkan uniform buffers
 *
 * Manages multiple uniform buffer instances (e.g., per-frame-in-flight).
 */
class VulkanUniformBufferSet final : public UniformBufferSet
{
public:
    /**
     * @brief Constructs uniform buffer set
     * @param buffer_size Per-buffer size in bytes
     * @param size Number of buffers
     * @param device Vulkan device
     */
    VulkanUniformBufferSet(size_t buffer_size, size_t size, const VulkanDevice& device);

    /**
     * @brief Gets buffer at index
     * @param index Buffer index
     * @return Uniform buffer reference
     */
    Reference<UniformBuffer> get(size_t index) override;

    /**
     * @brief Sets buffer at index
     * @param buffer Uniform buffer
     * @param index Buffer index
     */
    void set(const Reference<UniformBuffer>& buffer, size_t index) override;

    /**
     * @brief Uploads data to buffer at index
     * @param data CPU buffer
     * @param offset Byte offset
     */
    void set_data(Buffer data, size_t offset) override;

    /** @brief Gets CPU buffer (const) */
    const Buffer& get_data() const override;

private:
    std::unordered_map<size_t, Reference<VulkanUniformBuffer>> buffers;

    [[maybe_unused]] const VulkanDevice& device;
};
} // portal
