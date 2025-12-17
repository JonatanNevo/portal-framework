//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_pipeline.h"

#include "pipeline_builder.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"

#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_render_target.h"

namespace portal::renderer::vulkan
{
VulkanPipeline::VulkanPipeline(const PipelineProperties& prop, const VulkanContext& context) : context(context), prop(prop)
{
    PORTAL_ASSERT(prop.shader, "Invalid pipeline shader");

    initialize();
    LOG_TRACE("PIPELINE CREATED {}", prop.debug_name);
}

VulkanPipeline::~VulkanPipeline()
{
    // TODO: submit resources for destruction?
    LOG_TRACE("PIPELINE DEAD {}", prop.debug_name);
}

PipelineProperties& VulkanPipeline::get_properties()
{
    return prop;
}

const PipelineProperties& VulkanPipeline::get_properties() const
{
    return prop;
}

void VulkanPipeline::initialize()
{
    PipelineBuilder builder;

    auto& device = context.get_device();

    auto shader = reference_cast<VulkanShaderVariant>(prop.shader);

    // Create the pipeline layout used to generate the rendering pipelines that are based on this descriptor set layout
    // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
    auto layouts = shader->get_descriptor_layouts();
    auto& push_constants = shader->get_push_constant_ranges();

    const vk::PipelineLayoutCreateInfo pipeline_layout_info = {
        .setLayoutCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(push_constants.size()),
        .pPushConstantRanges = push_constants.data()
    };

    pipeline_layout = device.create_pipeline_layout(pipeline_layout_info);

    builder.set_layout(pipeline_layout)
           .set_input_topology(to_primitive_topology(prop.topology))
           .set_polygon_mode(prop.wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill)
           .set_cull_mode(prop.backface_culling ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise)
           .set_line_width(prop.line_width); // this is dynamic;

    const size_t color_attachment_count = prop.attachments.attachment_images.size() - 1;

    builder.set_color_attachment_number(color_attachment_count);
    std::vector<ImageFormat> color_formats;
    auto depth_format = ImageFormat::None;
    color_formats.reserve(color_attachment_count);

    const auto& atts = prop.attachments.attachment_images;

    size_t color_index = 0;
    for (size_t i = 0; i < atts.size(); ++i)
    {
        const auto& attachment_prop = atts[i];

        if (utils::is_depth_format(attachment_prop.format))
        {
            if (depth_format != ImageFormat::None)
                PORTAL_ASSERT(depth_format == attachment_prop.format, "Multiple depth formats not supported");
            depth_format = attachment_prop.format;
            continue;
        }

        color_formats.push_back(attachment_prop.format);

        const auto blend_mode = prop.attachments.blend_mode == BlendMode::None
                                    ? attachment_prop.blend_mode
                                    : prop.attachments.blend_mode;
        builder.set_blend(color_index, prop.attachments.blend, blend_mode);
        color_index++;
    }

    builder.set_color_attachment_formats(color_formats)
           .set_depth_format(depth_format);

    if (prop.depth_test)
        builder.enable_depth_stencil(prop.depth_write, prop.depth_compare_operator);
    else
        builder.disable_depth_stencil();

    // TODO: enable?
    builder.disable_multisampling();

    // TODO: vertex binding??

    builder.add_shader(*shader);
    pipeline = device.create_pipeline(builder);
    device.set_debug_name(pipeline, prop.debug_name);
}

Reference<ShaderVariant> VulkanPipeline::get_shader() const
{
    return prop.shader;
}

bool VulkanPipeline::is_dynamic_line_width() const
{
    return prop.topology == PrimitiveTopology::Lines || prop.topology == PrimitiveTopology::LineStrip || prop.wireframe;
}

vk::Pipeline VulkanPipeline::get_vulkan_pipeline()
{
    return pipeline;
}

vk::PipelineLayout VulkanPipeline::get_vulkan_pipeline_layout()
{
    return pipeline_layout;
}
}
