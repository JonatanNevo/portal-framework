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

/**
 * @class PipelineBuilder
 * @brief Builder for creating Vulkan graphics pipelines with dynamic rendering
 *
 * Provides method chaining API for configuring all pipeline state (shaders, vertex input,
 * rasterization, depth/stencil, blending, etc.). Uses VK_KHR_dynamic_rendering instead of
 * render passes, specifying color/depth formats at pipeline creation time.
 *
 * Default state:
 * - Viewport/scissor: Dynamic
 * - Topology: Triangle list
 * - Polygon mode: Fill
 * - Cull mode: Back-face culling (counter-clockwise front)
 * - Line width: 1.0
 * - Multisampling: Disabled
 *
 * Usage:
 * @code
 * auto pipeline = PipelineBuilder()
 *     .add_shader(vertex_shader)
 *     .add_shader(fragment_shader)
 *     .set_vertex_bindings(bindings)
 *     .set_vertex_attributes(attributes)
 *     .enable_depth_stencil(true, DepthCompareOperator::Less)
 *     .set_color_attachment_formats(formats)
 *     .set_layout(pipeline_layout)
 *     .build(device.get_handle(), pipeline_cache);
 * @endcode
 */
class PipelineBuilder
{
public:
    /**
     * @brief Adds shader stage to pipeline
     * @param shader Shader variant (vertex, fragment, compute, etc.)
     * @return Reference to this builder
     */
    PipelineBuilder& add_shader(const VulkanShaderVariant& shader);

    /**
     * @brief Sets vertex input bindings
     * @param descriptions Vertex binding descriptions (stride, input rate)
     * @return Reference to this builder
     */
    PipelineBuilder& set_vertex_bindings(const std::vector<vk::VertexInputBindingDescription>& descriptions);

    /**
     * @brief Sets vertex input attributes
     * @param attribute_descriptions Vertex attribute descriptions (location, binding, format, offset)
     * @return Reference to this builder
     */
    PipelineBuilder& set_vertex_attributes(const std::vector<vk::VertexInputAttributeDescription>& attribute_descriptions);

    /**
     * @brief Sets input topology
     * @param topology Primitive topology (eTriangleList, eLineStrip, ePointList, etc.)
     * @return Reference to this builder
     */
    PipelineBuilder& set_input_topology(vk::PrimitiveTopology topology);

    /**
     * @brief Sets polygon mode
     * @param mode Polygon mode (eFill, eLine, ePoint)
     * @return Reference to this builder
     */
    PipelineBuilder& set_polygon_mode(vk::PolygonMode mode);

    /**
     * @brief Sets culling mode and front face winding
     * @param cull_mode Cull mode flags (eNone, eFront, eBack, eFrontAndBack)
     * @param front_face Front face winding (eCounterClockwise or eClockwise)
     * @return Reference to this builder
     */
    PipelineBuilder& set_cull_mode(vk::CullModeFlags cull_mode, vk::FrontFace front_face);

    /**
     * @brief Sets line width for line primitives
     * @param line_width Line width (default 1.0)
     * @return Reference to this builder
     */
    PipelineBuilder& set_line_width(float line_width);

    /**
     * @brief Disables multisampling (sample count = 1)
     * @return Reference to this builder
     */
    PipelineBuilder& disable_multisampling();

    /**
     * @brief Enables multisampling (MSAA)
     * @param samples Rasterization sample count (e2/e4/e8/...)
     * @param enable_sample_shading Enables per-sample shading (requires device feature)
     * @param min_sample_shading Minimum fraction of samples shaded when sample shading is enabled
     * @param alpha_to_coverage Enables alpha-to-coverage (useful for cutout transparency with MSAA)
     * @param alpha_to_one Enables alpha-to-one (rarely used)
     * @return Reference to this builder
     */
    PipelineBuilder& enable_multisampling(
        vk::SampleCountFlagBits samples,
        bool enable_sample_shading = false,
        float min_sample_shading = 1.0f,
        bool alpha_to_coverage = false,
        bool alpha_to_one = false
    );

    /**
     * @brief Enables depth/stencil testing
     * @param depth_write_enable Whether depth writes are enabled
     * @param depth_compare_op Depth comparison operator (Less, Greater, etc.)
     * @return Reference to this builder
     */
    PipelineBuilder& enable_depth_stencil(bool depth_write_enable, DepthCompareOperator depth_compare_op);

    /**
     * @brief Disables depth/stencil testing
     * @return Reference to this builder
     */
    PipelineBuilder& disable_depth_stencil();

    /**
     * @brief Sets number of color attachments
     * @param number Number of color attachments
     * @return Reference to this builder
     */
    PipelineBuilder& set_color_attachment_number(size_t number);

    /**
     * @brief Sets additive blending for color attachment
     * @param index Color attachment index
     * @return Reference to this builder
     */
    PipelineBuilder& set_blending_additive(size_t index);

    /**
     * @brief Sets alpha blending for color attachment
     * @param index Color attachment index
     * @return Reference to this builder
     */
    PipelineBuilder& set_blending_alpha(size_t index);

    /**
     * @brief Sets blending mode for color attachment
     * @param index Color attachment index
     * @param enable Whether blending is enabled
     * @param blend_mode Blend mode
     * @return Reference to this builder
     */
    PipelineBuilder& set_blend(size_t index, bool enable, BlendMode blend_mode);

    /**
     * @brief Disables color blending for attachment(s)
     * @param index Attachment index (-1 for all attachments)
     * @return Reference to this builder
     */
    PipelineBuilder& disable_color_blending(int index = -1);

    /**
     * @brief Sets color attachment formats for dynamic rendering
     * @param formats Vector of color attachment formats
     * @return Reference to this builder
     */
    PipelineBuilder& set_color_attachment_formats(std::vector<ImageFormat>& formats);

    /**
     * @brief Sets depth attachment format for dynamic rendering
     * @param depth_format Depth format
     * @return Reference to this builder
     */
    PipelineBuilder& set_depth_format(ImageFormat depth_format);

    /**
     * @brief Sets pipeline layout
     * @param layout Pipeline layout (descriptor set layouts, push constants)
     * @return Reference to this builder
     */
    PipelineBuilder& set_layout(vk::raii::PipelineLayout& layout);

    /**
     * @brief Sets pipeline debug name
     * @param debug_name Debug name for GPU debuggers
     * @return Reference to this builder
     */
    PipelineBuilder& set_name(const StringId& debug_name);

    /**
     * @brief Builds the graphics pipeline
     * @param device Vulkan device
     * @param pipeline_cache Pipeline cache for PSO reuse
     * @return Created pipeline
     */
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
