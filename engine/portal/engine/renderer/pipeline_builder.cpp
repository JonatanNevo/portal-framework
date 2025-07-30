//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "pipeline_builder.h"

namespace portal::vulkan
{

PipelineBuilder& PipelineBuilder::set_shaders(vk::raii::ShaderModule& vertex_shader, vk::raii::ShaderModule& fragment_shader)
{
    shader_stages.clear();
    shader_stages.push_back(
        {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = vertex_shader,
            .pName = "vert_main" // TODO: get from some abstract reflected shader class
        }
        );

    shader_stages.push_back(
        {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = fragment_shader,
            .pName = "frag_main" // TODO: get from some abstract reflected shader class
        }
        );


    return *this;
}

PipelineBuilder& PipelineBuilder::add_shader(const vk::raii::ShaderModule& module, const vk::ShaderStageFlagBits stage, std::string_view entry_point)
{
    shader_stages.push_back(
        {
            .stage = stage,
            .module = module,
            .pName = entry_point.data()
        }
        );
    return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_bindings(const std::vector<vk::VertexInputBindingDescription>& descriptions)
{
    vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(descriptions.size());
    vertex_input_state.pVertexBindingDescriptions = descriptions.data();
    return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_attributes(const std::vector<vk::VertexInputAttributeDescription>& attribute_descriptions)
{
    vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_state.pVertexAttributeDescriptions = attribute_descriptions.data();
    return *this;
}

PipelineBuilder& PipelineBuilder::set_input_topology(const vk::PrimitiveTopology topology)
{
    input_assembly.topology = topology;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_polygon_mode(const vk::PolygonMode mode)
{
    rasterization.polygonMode = mode;
    rasterization.lineWidth = 1.0f;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_cull_mode(const vk::CullModeFlags cull_mode, const vk::FrontFace front_face)
{
    rasterization.cullMode = cull_mode;
    rasterization.frontFace = front_face;
    return *this;
}

PipelineBuilder& PipelineBuilder::disable_multisampling()
{
    multisampling.sampleShadingEnable = false;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1; // Disable multisampling
    multisampling.minSampleShading = 1.f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = false;
    multisampling.alphaToOneEnable = false;
    return *this;
}

PipelineBuilder& PipelineBuilder::enable_depth_stencil(const bool depth_write_enable, const vk::CompareOp depth_compare_op)
{
    depth_stencil.depthTestEnable = true;
    depth_stencil.depthWriteEnable = depth_write_enable;
    depth_stencil.depthCompareOp = depth_compare_op;
    depth_stencil.depthBoundsTestEnable = false;
    depth_stencil.stencilTestEnable = false;
    depth_stencil.front = {};
    depth_stencil.back = {};
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;
    return *this;
}

PipelineBuilder& PipelineBuilder::disable_depth_stencil()
{
    depth_stencil.depthTestEnable = false;
    depth_stencil.depthWriteEnable = false;
    depth_stencil.depthCompareOp = vk::CompareOp::eNever;
    depth_stencil.depthBoundsTestEnable = false;
    depth_stencil.stencilTestEnable = false;
    depth_stencil.front = {};
    depth_stencil.back = {};
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;
    return *this;
}

constexpr auto color_write_mask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
    vk::ColorComponentFlagBits::eA;

PipelineBuilder& PipelineBuilder::enable_blending_additive()
{
    color_blend_attachment.blendEnable = true;
    color_blend_attachment.colorWriteMask = color_write_mask;
    color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOne;
    color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
    color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
    return *this;
}

PipelineBuilder& PipelineBuilder::enable_blending_alpha()
{
    color_blend_attachment.blendEnable = true;
    color_blend_attachment.colorWriteMask = color_write_mask;
    color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
    color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
    return *this;
}

PipelineBuilder& PipelineBuilder::disable_color_blending()
{
    color_blend_attachment.blendEnable = false;
    color_blend_attachment.colorWriteMask = color_write_mask;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_color_attachment_format(const vk::Format format)
{
    color_attachment_format = format;

    // connect the format to the pipeline_rendering_create_info structure
    pipeline_rendering_create_info.colorAttachmentCount = 1;
    pipeline_rendering_create_info.pColorAttachmentFormats = &color_attachment_format;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_format(const vk::Format format)
{
    pipeline_rendering_create_info.depthAttachmentFormat = format;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_layout(vk::raii::PipelineLayout& layout)
{
    pipeline_layout = &layout;
    return *this;
}

vk::raii::Pipeline PipelineBuilder::build(const vk::raii::Device& device)
{
    vk::PipelineDynamicStateCreateInfo dynamic_state{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };

    vk::PipelineViewportStateCreateInfo viewport_state{
        .viewportCount = 1,
        .scissorCount = 1
    };

    const vk::GraphicsPipelineCreateInfo pipeline_info{
        .pNext = &pipeline_rendering_create_info,
        .stageCount = static_cast<uint32_t>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = *pipeline_layout,
        .renderPass = nullptr
    };

    return device.createGraphicsPipeline(nullptr, pipeline_info);
}

} // portal
