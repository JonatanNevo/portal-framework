//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <functional>

#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/allocated_buffer.h"
#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/renderer/pipeline_builder.h"

namespace portal::vulkan {
struct DescriptorWriter;
struct ImageBuilder;
class AllocatedImage;
}

namespace portal::resources
{

vk::ShaderStageFlagBits to_vk_shader_stage(ShaderStage stage);

// TODO: find a better name?
// TODO: Make this virtual and not dependent on vulkan
/**
 * In charge of providing an interface for the loaders to do actions on the GPU to move data in and out of the GPU.
 */
class GpuContext
{
public:
    GpuContext(vk::raii::Device& device,
        vk::raii::CommandBuffer& commandBuffer,
        vk::raii::Queue& submit_queue,
        vulkan::AllocatedImage& draw_image,
        vulkan::AllocatedImage& depth_image,
        const std::vector<vk::DescriptorSetLayout>& global_descriptor_layouts
        );
    virtual ~GpuContext() = default;

    [[nodiscard]] virtual vulkan::AllocatedBuffer create_buffer(vulkan::BufferBuilder builder) const;
    [[nodiscard]] virtual std::shared_ptr<vulkan::AllocatedBuffer> create_buffer_shared(vulkan::BufferBuilder builder) const;

    virtual vulkan::AllocatedImage create_image(void* data, vulkan::ImageBuilder image_builder) const;
    virtual std::shared_ptr<vulkan::AllocatedImage> create_image_shared(void* data, vulkan::ImageBuilder image_builder) const;
    [[nodiscard]] virtual vk::raii::Sampler create_sampler(vk::SamplerCreateInfo create_info) const;
    virtual vk::raii::DescriptorSetLayout create_descriptor_set_layout(vulkan::DescriptorLayoutBuilder& builder);
    virtual vk::raii::DescriptorSet create_descriptor_set(const vk::DescriptorSetLayout& layout);
    virtual vk::raii::PipelineLayout create_pipeline_layout(const vk::PipelineLayoutCreateInfo& pipeline_layout_info);
    virtual vk::raii::ShaderModule create_shader_module(Buffer code);
    virtual vk::raii::Pipeline create_pipeline(vulkan::PipelineBuilder builder);

    virtual std::vector<vk::DescriptorSetLayout>& get_global_descriptor_layouts();
    virtual void write_descriptor_sets(Ref<Shader> shader, std::vector<vk::raii::DescriptorSet>& sets, size_t skip);

    virtual vk::Format get_draw_image_format();
    virtual vk::Format get_depth_format();
    void immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function) const;

private:
    // Images
    void populate_image(const void* data, vulkan::AllocatedImage& image) const;
    void generate_mipmaps(const vk::raii::CommandBuffer& active_cmd, const vulkan::AllocatedImage& image) const;

private:
    vulkan::DescriptorAllocator descriptor_allocator;
    std::vector<vk::DescriptorSetLayout> global_descriptor_layouts;
    vk::raii::Fence fence = nullptr;
    vk::raii::Device& device;
    vk::raii::CommandBuffer& command_buffer;
    vk::raii::Queue& submit_queue;
    vulkan::AllocatedImage& draw_image;
    vulkan::AllocatedImage& depth_image;
};

} // portal
