//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "material.h"
#include <portal/core/glm.h>

#include "portal/engine/renderer/allocated_buffer.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/descriptor_writer.h"

namespace portal
{

struct GLTFMetallicRoughness
{
    MaterialPipeline opaque_pipeline{};
    MaterialPipeline transparent_pipeline{};

    vk::raii::DescriptorSetLayout material_layout = nullptr;

    struct MaterialConsts
    {
        glm::vec4 color_factors;
        glm::vec4 metal_rough_factors;
        //padding, we need it anyway for uniform buffers
        glm::vec4 extra[14];
    };

    struct MaterialResources
    {
        vulkan::AllocatedImage* color_image;
        vk::raii::Sampler* color_sampler;
        vulkan::AllocatedImage* metallic_roughness_image;
        vk::raii::Sampler* metallic_roughness_sampler;
        vulkan::AllocatedBuffer* data_buffer;
        uint32_t data_buffer_offset;
    };

    vulkan::DescriptorWriter writer{};

    void build_pipelines(
        vk::raii::Device& device,
        const vk::DescriptorSetLayout& global_layout,
        vk::Format color_format,
        vk::Format depth_format
        );
    void clear_resources(vk::raii::Device& device);

    MaterialInstance write_material(const vk::raii::Device& device, MaterialPass pass, MaterialResources& resources, vulkan::DescriptorAllocator& desc_allocator);
};

} // portal
