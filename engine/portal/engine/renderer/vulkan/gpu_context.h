//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <functional>

#include <vulkan/vulkan_raii.hpp>

#include "allocated_buffer.h"
#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/vulkan/pipeline_builder.h"

namespace portal::vulkan
{
class DescriptorLayoutBuilder;
struct DescriptorWriter;
}

namespace portal::renderer::vulkan
{

struct ImageBuilder;
class AllocatedImage;
class VulkanImage;

// TODO: find a better name?
// TODO: Make this virtual and not dependent on vulkan
/**
 * In charge of providing an interface for the loaders to do actions on the GPU to move data in and out of the GPU.
 */
class GpuContext
{
public:
    GpuContext(
        vk::raii::Device& device,
        const Ref<RenderTarget>& render_target,
        const std::vector<vk::DescriptorSetLayout>& global_descriptor_layouts
        );
    virtual ~GpuContext() = default;

    virtual vk::raii::DescriptorSet create_descriptor_set(const vk::DescriptorSetLayout& layout);

    virtual std::vector<vk::DescriptorSetLayout>& get_global_descriptor_layouts();
    virtual void write_descriptor_set(portal::vulkan::DescriptorWriter& writer, vk::raii::DescriptorSet& set);

    vk::Device get_device() const;

    Ref<RenderTarget> get_render_target() const;

private:
    Ref<RenderTarget> render_target;
    portal::vulkan::DescriptorAllocator descriptor_allocator;
    std::vector<vk::DescriptorSetLayout> global_descriptor_layouts;
    vk::raii::Device& device;
};

} // portal
