//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_pipeline.h"

#include "pipeline_builder.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"

#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_render_target.h"

namespace portal::renderer::vulkan
{

VulkanPipeline::VulkanPipeline(const pipeline::Specification& spec, const Ref<VulkanContext>& context) : context(context), spec(spec)
{
    PORTAL_ASSERT(spec.shader, "Invalid pipeline shader");
    PORTAL_ASSERT(context, "Invalid GPU context");

    initialize();
    // Register dependency between shader and pipeline?
}

VulkanPipeline::~VulkanPipeline()
{
    // TODO: submit resources for destruction?
}

pipeline::Specification& VulkanPipeline::get_spec()
{
    return spec;
}

const pipeline::Specification& VulkanPipeline::get_spec() const
{
    return spec;
}

void VulkanPipeline::initialize()
{
    PipelineBuilder builder;

    auto device = context->get_device();

    auto shader = spec.shader.as<VulkanShader>;

    // Create the pipeline layout used to generate the rendering pipelines that are based on this descriptor set layout
    // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
    std::vector<vk::DescriptorSetLayout> layouts = shader->get_descriptor_layouts();
    std::vector<vk::PushConstantRange> push_constants = shader->get_push_constants();

    const vk::PipelineLayoutCreateInfo pipeline_layout_info = {
        .setLayoutCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(push_constants.size()),
        .pPushConstantRanges = push_constants.data()
    };

    pipeline_layout = device->create_pipeline_layout(pipeline_layout_info);

    builder.set_layout(pipeline_layout)
           .set_input_topology(to_primitive_topology(spec.topology))
           .set_polygon_mode(spec.wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill)
           .set_cull_mode(spec.backface_culling ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise)
           .set_line_width(spec.line_width); // this is dynamic;

    auto render_target = spec.render_target.as<VulkanRenderTarget>();
    const size_t color_attachment_count = render_target->get_color_attachment_count();

    builder.set_color_attachment_number(color_attachment_count);
    std::vector<ImageFormat> color_formats;
    ImageFormat depth_format = ImageFormat::None;

    color_formats.reserve(color_attachment_count);

    for (size_t i = 0; i < color_attachment_count; ++i)
    {
        if (!render_target->get_spec().blend)
            break;

        const auto& attachment_spec = render_target->get_spec().attachments.attachments[i];

        if (utils::is_depth_format(attachment_spec.format))
        {
            PORTAL_ASSERT(depth_format != ImageFormat::None && depth_format == attachment_spec.format, "Multiple depth formats not supported");
            depth_format = attachment_spec.format;
        }
        else
            color_formats.push_back(attachment_spec.format);

        const auto blend_mode = render_target->get_spec().blend_mode == render_target::BlendMode::None
            ? attachment_spec.blend_mode
            : render_target->get_spec().blend_mode;
        builder.set_blend(i, render_target->get_spec().blend, blend_mode);
    }

    builder.set_color_attachment_formats(color_formats)
           .set_depth_format(depth_format);

    if (spec.depth_test)
        builder.enable_depth_stencil(spec.depth_write, spec.depth_compare_operator);
    else
        builder.disable_depth_stencil();

    // TODO: enable?
    builder.disable_multisampling();

    // TODO: vertex binding??

    // builder.add_shader(shader);
    auto created_pipeline = device->create_pipeline(builder);
    pipeline = std::move(created_pipeline);
}

Ref<Shader> VulkanPipeline::get_shader() const
{
    return spec.shader;
}

bool VulkanPipeline::is_dynamic_line_width() const
{
    return spec.topology == PrimitiveTopology::Lines || spec.topology == PrimitiveTopology::LineStrip || spec.wireframe;
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
