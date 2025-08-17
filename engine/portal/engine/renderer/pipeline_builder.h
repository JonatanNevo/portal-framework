//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/resources/resources/shader.h"

namespace portal::vulkan
{

class PipelineBuilder
{
public:
    PipelineBuilder& set_shaders(Ref<Shader> vertex_shader, Ref<Shader> fragment_shader);
    PipelineBuilder& add_shader(const vk::ShaderModule& module, vk::ShaderStageFlagBits stage, std::string_view entry_point);

    PipelineBuilder& set_vertex_bindings(const std::vector<vk::VertexInputBindingDescription>& descriptions);
    PipelineBuilder& set_vertex_attributes(const std::vector<vk::VertexInputAttributeDescription>& attribute_descriptions);

    PipelineBuilder& set_input_topology(vk::PrimitiveTopology topology);

    PipelineBuilder& set_polygon_mode(vk::PolygonMode mode);
    PipelineBuilder& set_cull_mode(vk::CullModeFlags cull_mode, vk::FrontFace front_face);

    PipelineBuilder& disable_multisampling();

    PipelineBuilder& enable_depth_stencil(bool depth_write_enable, vk::CompareOp depth_compare_op);
    PipelineBuilder& disable_depth_stencil();

    PipelineBuilder& enable_blending_additive();
    PipelineBuilder& enable_blending_alpha();
    PipelineBuilder& disable_color_blending();

    PipelineBuilder& set_color_attachment_format(vk::Format format);
    PipelineBuilder& set_depth_format(vk::Format format);

    PipelineBuilder& set_layout(vk::raii::PipelineLayout& layout);

    vk::raii::Pipeline build(const vk::raii::Device& device);

protected:
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {};

    std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineVertexInputStateCreateInfo vertex_input_state = {};

    vk::PipelineInputAssemblyStateCreateInfo input_assembly{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False
    };

    vk::PipelineRasterizationStateCreateInfo rasterization{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{};

    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};

    vk::PipelineColorBlendAttachmentState color_blend_attachment{};

    vk::PipelineColorBlendStateCreateInfo color_blending{
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment
    };

    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{};

    vk::raii::PipelineLayout* pipeline_layout = nullptr;

    vk::Format color_attachment_format = vk::Format::eUndefined;
};

} // portal
