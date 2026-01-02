//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/reference.h"
#include "portal/engine/renderer/descriptors/storage_buffer.h"
#include "portal/engine/renderer/vulkan/allocated_buffer.h"

namespace portal::renderer::vulkan
{
class VulkanDevice;

/**
 * @class VulkanStorageBuffer
 * @brief Vulkan storage buffer with VMA allocation
 *
 * GPU buffer for shader read-write access. Supports CPU-visible and GPU-only modes.
 * Resizable.
 */
class VulkanStorageBuffer final : public StorageBuffer
{
public:
    /**
     * @brief Constructs Vulkan storage buffer
     * @param properties Buffer properties (size, gpu_only)
     * @param device Vulkan device
     */
    VulkanStorageBuffer(const StorageBufferProperties& properties, const VulkanDevice& device);
    ~VulkanStorageBuffer() override;

    /**
     * @brief Uploads data to GPU
     * @param data CPU buffer
     * @param offset Byte offset
     */
    void set_data(Buffer data, size_t offset) override;

    /** @brief Gets CPU buffer (const) */
    const Buffer& get_data() const override;

    /**
     * @brief Resizes buffer (recreates GPU allocation)
     * @param new_size New size in bytes
     */
    void resize(size_t new_size) override;

    /** @brief Gets descriptor buffer info for binding */
    vk::DescriptorBufferInfo& get_descriptor_buffer_info();

private:
    /** @brief Releases GPU resources */
    void release();

    /** @brief Allocates GPU buffer */
    void init();

private:
    const VulkanDevice& device;
    StorageBufferProperties properties;

    AllocatedBuffer buffer;
    vk::DescriptorBufferInfo descriptor_buffer_info;

    Buffer local_storage = nullptr;
};

/**
 * @class VulkanStorageBufferSet
 * @brief Collection of Vulkan storage buffers
 *
 * Manages multiple storage buffer instances (e.g., per-frame-in-flight).
 */
class VulkanStorageBufferSet final : public StorageBufferSet
{
public:
    /**
     * @brief Constructs storage buffer set
     * @param buffer_size Per-buffer size in bytes
     * @param size Number of buffers
     * @param device Vulkan device
     */
    VulkanStorageBufferSet(size_t buffer_size, size_t size, const VulkanDevice& device);

    /**
     * @brief Gets buffer at index
     * @param index Buffer index
     * @return Storage buffer reference
     */
    Reference<StorageBuffer> get(size_t index) override;

    /**
     * @brief Sets buffer at index
     * @param buffer Storage buffer
     * @param index Buffer index
     */
    void set(const Reference<StorageBuffer>& buffer, size_t index) override;

    /**
     * @brief Uploads data to buffer at index
     * @param data CPU buffer
     * @param offset Byte offset
     */
    void set_data(Buffer data, size_t offset) override;

    /** @brief Gets CPU buffer (const) */
    const Buffer& get_data() const override;

private:
    std::unordered_map<size_t, Reference<VulkanStorageBuffer>> buffers;
};
} // portal
