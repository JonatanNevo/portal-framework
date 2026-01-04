//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "pipeline_builder.h"

#include <ranges>

#include "vulkan_instance.h"
#include "portal/engine/renderer/render_target/render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_common.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"

namespace portal::renderer::vulkan
{
PipelineBuilder& PipelineBuilder::add_shader(const VulkanShaderVariant& shader)
{
    const std::vector<vk::PipelineShaderStageCreateInfo>& shader_create_infos = shader.get_shader_stage_create_infos();
    shader_stages.insert(shader_stages.end(), shader_create_infos.begin(), shader_create_infos.end());
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

PipelineBuilder& PipelineBuilder::set_line_width(const float line_width)
{
    rasterization.lineWidth = line_width;
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

PipelineBuilder& PipelineBuilder::enable_depth_stencil(const bool depth_write_enable, const DepthCompareOperator depth_compare_op)
{
    depth_stencil.depthTestEnable = true;
    depth_stencil.depthWriteEnable = depth_write_enable;
    depth_stencil.depthCompareOp = to_compare_op(depth_compare_op);
    depth_stencil.depthBoundsTestEnable = false;
    depth_stencil.stencilTestEnable = false;
    depth_stencil.front = vk::StencilOpState{};
    depth_stencil.back = vk::StencilOpState{};
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
    depth_stencil.front = vk::StencilOpState{};
    depth_stencil.back = vk::StencilOpState{};
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_color_attachment_number(size_t number)
{
    PORTAL_ASSERT(color_blend_attachments.size() == 0, "Color attachment already set");
    color_blend_attachments.resize(number);
    return *this;
}

constexpr auto color_write_mask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
    vk::ColorComponentFlagBits::eA;

PipelineBuilder& PipelineBuilder::set_blending_additive(const size_t index)
{
    PORTAL_ASSERT(index < color_blend_attachments.size(), "Color attachment index out of range");

    auto& attachment = color_blend_attachments[index];
    attachment.blendEnable = true;
    attachment.colorWriteMask = color_write_mask;
    attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    attachment.dstColorBlendFactor = vk::BlendFactor::eOne;
    attachment.colorBlendOp = vk::BlendOp::eAdd;
    attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    attachment.alphaBlendOp = vk::BlendOp::eAdd;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_blending_alpha(const size_t index)
{
    PORTAL_ASSERT(index < color_blend_attachments.size(), "Color attachment index out of range");
    auto& attachment = color_blend_attachments[index];
    attachment.blendEnable = true;
    attachment.colorWriteMask = color_write_mask;
    attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    attachment.colorBlendOp = vk::BlendOp::eAdd;
    attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    attachment.alphaBlendOp = vk::BlendOp::eAdd;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_blend(const size_t index, const bool enable, const BlendMode blend_mode)
{
    PORTAL_ASSERT(index < color_blend_attachments.size(), "Color attachment index out of range");
    auto& attachment = color_blend_attachments[index];
    attachment.blendEnable = enable;
    attachment.colorWriteMask = color_write_mask;

    attachment.colorBlendOp = vk::BlendOp::eAdd;
    attachment.alphaBlendOp = vk::BlendOp::eAdd;

    attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;

    switch (blend_mode)
    {
    case BlendMode::Additive:
        attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        attachment.dstColorBlendFactor = vk::BlendFactor::eOne;
        attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        break;
    case BlendMode::OneZero:
        attachment.srcColorBlendFactor = vk::BlendFactor::eOne;
        attachment.dstColorBlendFactor = vk::BlendFactor::eZero;
        break;
    case BlendMode::SrcAlphaOneMinusSrcAlpha:
        attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        attachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        attachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        break;
    case BlendMode::ZeroSrcColor:
        attachment.srcColorBlendFactor = vk::BlendFactor::eZero;
        attachment.dstColorBlendFactor = vk::BlendFactor::eSrcColor;
        break;
    default:
        PORTAL_ASSERT(false, "Unknown blend mode");
    }
    return *this;
}

PipelineBuilder& PipelineBuilder::disable_color_blending(const int index)
{
    if (index == -1)
    {
        PORTAL_ASSERT(color_blend_attachments.size() == 0, "No color attachment set");

        auto& attachment = color_blend_attachments.emplace_back();
        attachment.blendEnable = false;
        attachment.colorWriteMask = color_write_mask;
        return *this;
    }
    PORTAL_ASSERT(static_cast<size_t>(index) < color_blend_attachments.size(), "Color attachment index out of range");
    auto& attachment = color_blend_attachments[index];
    attachment.blendEnable = false;
    attachment.colorWriteMask = color_write_mask;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_color_attachment_formats(std::vector<ImageFormat>& formats)
{
    auto transformed_view = formats | std::views::transform(
        [](const ImageFormat format)
        {
            return to_format(format);
        }
    );
    color_formats = std::ranges::to<std::vector<vk::Format>>(transformed_view);

    pipeline_rendering_create_info.colorAttachmentCount = static_cast<uint32_t>(color_formats.size());
    pipeline_rendering_create_info.pColorAttachmentFormats = color_formats.data();
    return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_format(const ImageFormat depth_format)
{
    pipeline_rendering_create_info.depthAttachmentFormat = to_format(depth_format);
    return *this;
}

PipelineBuilder& PipelineBuilder::set_layout(vk::raii::PipelineLayout& layout)
{
    pipeline_layout = &layout;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_name(const StringId& debug_name)
{
    name = debug_name;
    return *this;
}

vk::raii::Pipeline PipelineBuilder::build(const vk::raii::Device& device, const vk::raii::PipelineCache& pipeline_cache)
{
    vk::PipelineDynamicStateCreateInfo dynamic_state{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };

    vk::PipelineViewportStateCreateInfo viewport_state{
        .viewportCount = 1,
        .scissorCount = 1
    };

    PORTAL_ASSERT(!color_blend_attachments.empty(), "No color attachment found");

    color_blending = {
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = static_cast<uint32_t>(color_blend_attachments.size()),
        .pAttachments = color_blend_attachments.data()
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

    auto&& pipeline = device.createGraphicsPipeline(pipeline_cache, pipeline_info);

    if constexpr (ENABLE_VALIDATION_LAYERS)
    {
        if (name != INVALID_STRING_ID)
        {
            device.setDebugUtilsObjectNameEXT(
                vk::DebugUtilsObjectNameInfoEXT{
                    .objectType = vk::ObjectType::ePipeline,
                    .objectHandle = VK_HANDLE_CAST(pipeline),
                    .pObjectName = name.string.data()
                }
            );
        }
    }
    return pipeline;
}
} // portal
