//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/pipeline/pipeline_types.h"
#include "portal/engine/renderer/render_target/render_target.h"

namespace portal::renderer::vulkan
{

class VulkanShaderVariant;

class PipelineBuilder
{
public:
    PipelineBuilder& add_shader(const VulkanShaderVariant& shader);

    PipelineBuilder& set_vertex_bindings(const std::vector<vk::VertexInputBindingDescription>& descriptions);
    PipelineBuilder& set_vertex_attributes(const std::vector<vk::VertexInputAttributeDescription>& attribute_descriptions);

    PipelineBuilder& set_input_topology(vk::PrimitiveTopology topology);

    PipelineBuilder& set_polygon_mode(vk::PolygonMode mode);
    PipelineBuilder& set_cull_mode(vk::CullModeFlags cull_mode, vk::FrontFace front_face);
    PipelineBuilder& set_line_width(float line_width);

    PipelineBuilder& disable_multisampling();

    PipelineBuilder& enable_depth_stencil(bool depth_write_enable, DepthCompareOperator depth_compare_op);
    PipelineBuilder& disable_depth_stencil();

    PipelineBuilder& set_color_attachment_number(size_t number);
    PipelineBuilder& set_blending_additive(size_t index);
    PipelineBuilder& set_blending_alpha(size_t index);

    PipelineBuilder& set_blend(size_t index, bool enable, BlendMode blend_mode);
    PipelineBuilder& disable_color_blending(int index = -1);

    PipelineBuilder& set_color_attachment_formats(std::vector<ImageFormat>& formats);
    PipelineBuilder& set_depth_format(ImageFormat depth_format);

    PipelineBuilder& set_layout(vk::raii::PipelineLayout& layout);
    PipelineBuilder& set_name(const StringId& debug_name);

    vk::raii::Pipeline build(const vk::raii::Device& device, const vk::raii::PipelineCache& pipeline_cache);

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

    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;

    vk::PipelineColorBlendStateCreateInfo color_blending{};

    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{};

    vk::raii::PipelineLayout* pipeline_layout = nullptr;

    std::vector<vk::Format> color_formats;

    StringId name;
};

} // portal
