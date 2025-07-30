//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "gltf_metallic_roughness.h"

#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/renderer/pipeline_builder.h"
#include "portal/engine/renderer/pipelines.h"
#include "portal/engine/renderer/rendering_types.h"

namespace portal
{

void GLTFMetallicRoughness::build_pipelines(
    vk::raii::Device& device,
    const vk::DescriptorSetLayout& global_layout,
    const vk::Format color_format,
    const vk::Format depth_format
    )
{
    const auto module = vulkan::load_shader_module("../mesh.shading.slang.spv", device);

    vk::PushConstantRange matrix_range{
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(vulkan::GPUDrawPushConstants)
    };

    material_layout = vulkan::DescriptorLayoutBuilder{}
                      .add_binding(0, vk::DescriptorType::eUniformBuffer)
                      .add_binding(1, vk::DescriptorType::eCombinedImageSampler)
                      .add_binding(2, vk::DescriptorType::eCombinedImageSampler)
                      .build(device, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex);

    vk::DescriptorSetLayout layouts[] = {
        global_layout,
        material_layout
    };

    const vk::PipelineLayoutCreateInfo pipeline_layout_info{
        .setLayoutCount = 2,
        .pSetLayouts = layouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &matrix_range
    };

    opaque_pipeline.layout = device.createPipelineLayout(pipeline_layout_info);
    transparent_pipeline.layout = device.createPipelineLayout(pipeline_layout_info);

    // build the stage-create-info for both vertex and fragment stages. This lets
    // the pipeline know the shader modules per stage
    vulkan::PipelineBuilder builder;
    builder.add_shader(module, vk::ShaderStageFlagBits::eVertex, "vert_main")
           .add_shader(module, vk::ShaderStageFlagBits::eFragment, "frag_main")
           .set_input_topology(vk::PrimitiveTopology::eTriangleList)
           .set_polygon_mode(vk::PolygonMode::eFill)
           .set_cull_mode(vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise)
           .disable_multisampling()
           .disable_color_blending()
           .enable_depth_stencil(true, vk::CompareOp::eGreaterOrEqual)
           .set_color_attachment_format(color_format)
           .set_depth_format(depth_format);

    builder.set_layout(opaque_pipeline.layout);
    opaque_pipeline.pipeline = builder.build(device);

    builder.set_layout(transparent_pipeline.layout)
           .enable_blending_additive()
           .enable_depth_stencil(false, vk::CompareOp::eGreaterOrEqual);
    transparent_pipeline.pipeline = builder.build(device);
}

MaterialInstance GLTFMetallicRoughness::write_material(
    const vk::raii::Device& device,
    const MaterialPass pass,
    MaterialResources& resources,
    vulkan::DescriptorAllocator& desc_allocator
    )
{
    MaterialInstance instance;
    instance.pass_type = pass;
    if (pass == MaterialPass::Transparent)
    {
        instance.pipeline = &transparent_pipeline;
    }
    else
    {
        instance.pipeline = &opaque_pipeline;
    }

    instance.material_set = desc_allocator.allocate(material_layout);

    writer.clear();
    writer.write_buffer(0, *resources.data_buffer, sizeof(MaterialConsts), resources.data_buffer_offset, vk::DescriptorType::eUniformBuffer);
    writer.write_image(
        1,
        resources.color_image->get_view(),
        *resources.color_sampler,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::DescriptorType::eCombinedImageSampler
        );
    writer.write_image(
        2,
        resources.metallic_roughness_image->get_view(),
        *resources.metallic_roughness_sampler,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::DescriptorType::eCombinedImageSampler
        );
    writer.update_set(device, instance.material_set);
    return instance;
}

} // portal
