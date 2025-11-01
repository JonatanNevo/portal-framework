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

class VulkanStorageBuffer final : public StorageBuffer
{
public:
    VulkanStorageBuffer(const StorageBufferSpecification& spec, const VulkanDevice& device);
    ~VulkanStorageBuffer() override;

    void set_data(Buffer data, size_t offset) override;
    const Buffer& get_data() const override;
    void resize(size_t new_size) override;

    vk::DescriptorBufferInfo& get_descriptor_buffer_info();

private:
    void release();
    void init();


private:
    const VulkanDevice& device;
    StorageBufferSpecification spec;

    AllocatedBuffer buffer;
    vk::DescriptorBufferInfo descriptor_buffer_info;

    Buffer local_storage = nullptr;
};

class VulkanStorageBufferSet final : public StorageBufferSet
{
public:
    VulkanStorageBufferSet(size_t buffer_size, size_t size, const VulkanDevice& device);

    Reference<StorageBuffer> get(size_t index) override;
    void set(const Reference<StorageBuffer>& buffer, size_t index) override;

    void set_data(Buffer data, size_t offset) override;
    const Buffer& get_data() const override;

private:
    std::unordered_map<size_t, Reference<VulkanStorageBuffer>> buffers;
};

} // portal