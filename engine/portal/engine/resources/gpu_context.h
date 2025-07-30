//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <functional>

#include <vulkan/vulkan_raii.hpp>

namespace portal::vulkan {
struct ImageBuilder;
class AllocatedImage;
}

namespace portal::resources
{

// TODO: find a better name?
// TODO: Make this virtual and not dependent on vulkan
/**
 * In charge of providing an interface for the loaders to do actions on the GPU to move data in and out of the GPU.
 */
class GpuContext
{
public:
    GpuContext(vk::raii::Device& device, vk::raii::CommandBuffer& commandBuffer, vk::raii::Queue& submit_queue);
    virtual ~GpuContext() = default;

    virtual vulkan::AllocatedImage create_image(void* data, vulkan::ImageBuilder image_builder) const;
private:
    void immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function) const;

    // Images
    void populate_image(const void* data, vulkan::AllocatedImage& image) const;
    void generate_mipmaps(const vk::raii::CommandBuffer& active_cmd, const vulkan::AllocatedImage& image) const;

private:
    vk::raii::Fence fence = nullptr;
    vk::raii::Device& device;
    vk::raii::CommandBuffer& command_buffer;
    vk::raii::Queue& submit_queue;
};

} // portal
