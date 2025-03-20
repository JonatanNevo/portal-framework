//
// Created by Jonatan Nevo on 09/03/2025.
//

#include "pipeline.h"

#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/pipeline_layout.h"
#include "portal/application/vulkan/render_pass.h"

namespace portal::vulkan
{
Pipeline::Pipeline(Device& device): VulkanResource(nullptr, &device) {}

Pipeline::Pipeline(Pipeline&& other) noexcept: VulkanResource(std::move(other)), state(std::exchange(other.state, {})) {}

Pipeline::~Pipeline()
{
    if (has_handle())
        get_device_handle().destroyPipeline(get_handle());
}

const PipelineState& Pipeline::get_state() const
{
    return state;
}

ComputePipeline::ComputePipeline(Device& device, vk::PipelineCache pipeline_cache, PipelineState& pipeline_state)
    : Pipeline(device)
{
    auto* shader_module = pipeline_state.get_pipeline_layout().get_shader_modules().front();

    if (shader_module->get_stage() != vk::ShaderStageFlagBits::eCompute)
        throw std::runtime_error("The shader module must be a compute shader");

    // TODO: Move this to shader module class
    vk::ShaderModuleCreateInfo shader_create_info({}, shader_module->get_binary());
    auto vk_shader_module = get_device_handle().createShaderModule(shader_create_info);
    vk::PipelineShaderStageCreateInfo stage({}, shader_module->get_stage(), vk_shader_module, "main");

    device.get_debug_utils().set_debug_name(
        device.get_handle(),
        vk::ObjectType::eShaderModule,
        reinterpret_cast<uint64_t>(stage.module.operator VkShaderModule()),
        shader_module->get_debug_name().c_str()
    );

    // Create specialization info from tracked state.
    std::vector<uint8_t> data{};
    std::vector<vk::SpecializationMapEntry> map_entries{};

    const auto specialization_constant_state = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();
    for (const auto& [id, constants] : specialization_constant_state)
    {
        map_entries.emplace_back(id, static_cast<uint32_t>(data.size()), constants.size());
        data.insert(data.end(), constants.begin(), constants.end());
    }

    vk::SpecializationInfo specialization_info(map_entries.size(), map_entries.data(), data.size(), data.data());
    stage.pSpecializationInfo = &specialization_info;

    vk::ComputePipelineCreateInfo create_info({}, stage, pipeline_state.get_pipeline_layout().get_handle());
    set_handle(get_device_handle().createComputePipelines(pipeline_cache, {create_info}).value.front());
    get_device_handle().destroyShaderModule(vk_shader_module);

    state = pipeline_state;
}

GraphicsPipeline::GraphicsPipeline(Device& device, vk::PipelineCache pipeline_cache, PipelineState& pipeline_state)
    : Pipeline(device)
{
    std::vector<vk::ShaderModule> shader_modules;
    std::vector<vk::PipelineShaderStageCreateInfo> stage_create_infos;

    // Create specialization info from tracked state. This is shared by all shaders.
    std::vector<uint8_t> data{};
    std::vector<vk::SpecializationMapEntry> map_entries{};
    const auto specialization_constant_state = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();
    for (const auto& [id, constants] : specialization_constant_state)
    {
        map_entries.emplace_back(id, static_cast<uint32_t>(data.size()), constants.size());
        data.insert(data.end(), constants.begin(), constants.end());
    }
    vk::SpecializationInfo specialization_info(map_entries.size(), map_entries.data(), data.size(), data.data());

    // TODO: Move this to shader module class
    for (const auto* shader_module : pipeline_state.get_pipeline_layout().get_shader_modules())
    {
        vk::ShaderModuleCreateInfo shader_create_info({}, shader_module->get_binary());
        auto vk_shader_module = get_device_handle().createShaderModule(shader_create_info);

        device.get_debug_utils().set_debug_name(
            device.get_handle(),
            vk::ObjectType::eShaderModule,
            reinterpret_cast<uint64_t>(vk_shader_module.operator VkShaderModule()),
            shader_module->get_debug_name().c_str()
        );

        vk::PipelineShaderStageCreateInfo stage({}, shader_module->get_stage(), vk_shader_module, "main", &specialization_info);
        stage_create_infos.push_back(stage);
        shader_modules.push_back(vk_shader_module);
    }

    vk::PipelineVertexInputStateCreateInfo vertex_input_state(
        {},
        pipeline_state.get_vertex_input_state().bindings,
        pipeline_state.get_vertex_input_state().attributes
    );

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state(
        {},
        pipeline_state.get_input_assembly_state().topology,
        pipeline_state.get_input_assembly_state().primitive_restart_enable
    );

    vk::PipelineViewportStateCreateInfo viewport_state(
        {},
        pipeline_state.get_viewport_state().viewport_count,
        {},
        pipeline_state.get_viewport_state().scissor_count,
        {}
    );

    vk::PipelineRasterizationStateCreateInfo rasterization_state(
        {},
        pipeline_state.get_rasterization_state().depth_clamp_enable,
        pipeline_state.get_rasterization_state().rasterizer_discard_enable,
        pipeline_state.get_rasterization_state().polygon_mode,
        pipeline_state.get_rasterization_state().cull_mode,
        pipeline_state.get_rasterization_state().front_face,
        pipeline_state.get_rasterization_state().depth_bias_enable,
        1.f,
        1.f,
        1.f
    );


    vk::PipelineMultisampleStateCreateInfo multisample_state(
        {},
        pipeline_state.get_multisample_state().rasterization_samples,
        pipeline_state.get_multisample_state().sample_shading_enable,
        pipeline_state.get_multisample_state().min_sample_shading,
        {},
        pipeline_state.get_multisample_state().alpha_to_coverage_enable,
        pipeline_state.get_multisample_state().alpha_to_one_enable
    );
    if (pipeline_state.get_multisample_state().sample_mask)
    {
        multisample_state.pSampleMask = &pipeline_state.get_multisample_state().sample_mask;
    }

    vk::StencilOpState front(
        pipeline_state.get_depth_stencil_state().front.fail_op,
        pipeline_state.get_depth_stencil_state().front.pass_op,
        pipeline_state.get_depth_stencil_state().front.depth_fail_op,
        pipeline_state.get_depth_stencil_state().front.compare_op,
        ~0U,
        ~0U,
        ~0U
    );

    vk::StencilOpState back(
        pipeline_state.get_depth_stencil_state().back.fail_op,
        pipeline_state.get_depth_stencil_state().back.pass_op,
        pipeline_state.get_depth_stencil_state().back.depth_fail_op,
        pipeline_state.get_depth_stencil_state().back.compare_op,
        ~0U,
        ~0U,
        ~0U
    );

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state(
        {},
        pipeline_state.get_depth_stencil_state().depth_test_enable,
        pipeline_state.get_depth_stencil_state().depth_write_enable,
        pipeline_state.get_depth_stencil_state().depth_compare_op,
        pipeline_state.get_depth_stencil_state().depth_bounds_test_enable,
        pipeline_state.get_depth_stencil_state().stencil_test_enable,
        front,
        back
    );

    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments{pipeline_state.get_color_blend_state().attachments.size()};
    for (size_t i = 0; i < color_blend_attachments.size(); ++i)
    {
        color_blend_attachments[i] = pipeline_state.get_color_blend_state().attachments[i].to_vk_attachment();
    }

    vk::PipelineColorBlendStateCreateInfo color_blend_state(
        {},
        pipeline_state.get_color_blend_state().logic_op_enable,
        pipeline_state.get_color_blend_state().logic_op,
        color_blend_attachments,
        {1.f, 1.f, 1.f, 1.f}
    );

    constexpr std::array dynamic_states{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eLineWidth,
        vk::DynamicState::eDepthBias,
        vk::DynamicState::eBlendConstants,
        vk::DynamicState::eDepthBounds,
        vk::DynamicState::eStencilCompareMask,
        vk::DynamicState::eStencilWriteMask,
        vk::DynamicState::eStencilReference
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_states);

    vk::GraphicsPipelineCreateInfo create_info(
        {},
        stage_create_infos,
        &vertex_input_state,
        &input_assembly_state,
        nullptr,
        &viewport_state,
        &rasterization_state,
        &multisample_state,
        &depth_stencil_state,
        &color_blend_state,
        &dynamic_state,
        pipeline_state.get_pipeline_layout().get_handle(),
        pipeline_state.get_render_pass()->get_handle(),
        pipeline_state.get_subpass_index()
    );
    set_handle(get_device_handle().createGraphicsPipelines(pipeline_cache, {create_info}).value.front());

    for (auto shader_module: shader_modules)
    {
        get_device_handle().destroyShaderModule(shader_module);
    }

    state = pipeline_state;
}
} // portal
