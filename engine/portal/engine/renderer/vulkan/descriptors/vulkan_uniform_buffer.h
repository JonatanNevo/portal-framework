//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/descriptors/uniform_buffer.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"

namespace portal::renderer::vulkan
{

class VulkanUniformBuffer final : public UniformBuffer
{
public:
    VulkanUniformBuffer() = default;
    VulkanUniformBuffer(size_t size, const Ref<VulkanDevice>& device);
    ~VulkanUniformBuffer() override;

    void set_data(Buffer data, size_t offset) override;
    const Buffer& get_data() const override;

    const vk::DescriptorBufferInfo& get_descriptor_buffer_info() const;

private:
    void release();
    void init();

private:
    AllocatedBuffer buffer;
    size_t size;

    Buffer local_storage = nullptr;
    vk::DescriptorBufferInfo descriptor_buffer_info;

    Ref<VulkanDevice> device;
};

class VulkanUniformBufferSet final : public UniformBufferSet
{
public:
    VulkanUniformBufferSet(size_t buffer_size, size_t size, const Ref<VulkanDevice>& device);

    Ref<UniformBuffer> get(size_t index) override;
    void set(Ref<UniformBuffer> buffer, size_t index) override;

    void set_data(Buffer data, size_t offset) override;
    const Buffer& get_data() const override;

private:
    size_t size;
    std::unordered_map<size_t, Ref<VulkanUniformBuffer>> buffers;

    Ref<VulkanDevice> device;
};

} // portal
