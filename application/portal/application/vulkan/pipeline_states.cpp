//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "pipeline_states.h"

#include <portal/serialization/serialize.h>

#include "portal/application/vulkan/pipeline_layout.h"
#include "portal/application/vulkan/render_pass.h"


bool operator==(const VkVertexInputAttributeDescription& lhs, const VkVertexInputAttributeDescription& rhs)
{
	return std::tie(lhs.binding, lhs.format, lhs.location, lhs.offset) == std::tie(rhs.binding, rhs.format, rhs.location, rhs.offset);
}

bool operator==(const VkVertexInputBindingDescription& lhs, const VkVertexInputBindingDescription& rhs)
{
	return std::tie(lhs.binding, lhs.inputRate, lhs.stride) == std::tie(rhs.binding, rhs.inputRate, rhs.stride);
}

bool operator==(const portal::vulkan::ColorBlendAttachmentState& lhs, const portal::vulkan::ColorBlendAttachmentState& rhs)
{
	return std::tie(
			lhs.alpha_blend_op,
			lhs.blend_enable,
			lhs.color_blend_op,
			lhs.color_write_mask,
			lhs.dst_alpha_blend_factor,
			lhs.dst_color_blend_factor,
			lhs.src_alpha_blend_factor,
			lhs.src_color_blend_factor
		) ==
		std::tie(
			rhs.alpha_blend_op,
			rhs.blend_enable,
			rhs.color_blend_op,
			rhs.color_write_mask,
			rhs.dst_alpha_blend_factor,
			rhs.dst_color_blend_factor,
			rhs.src_alpha_blend_factor,
			rhs.src_color_blend_factor
		);
}

bool operator!=(const portal::vulkan::StencilOpState& lhs, const portal::vulkan::StencilOpState& rhs)
{
	return std::tie(lhs.compare_op, lhs.depth_fail_op, lhs.fail_op, lhs.pass_op) != std::tie(
		rhs.compare_op,
		rhs.depth_fail_op,
		rhs.fail_op,
		rhs.pass_op
	);
}

bool operator!=(const portal::vulkan::VertexInputState& lhs, const portal::vulkan::VertexInputState& rhs)
{
	return lhs.attributes != rhs.attributes || lhs.bindings != rhs.bindings;
}

bool operator!=(const portal::vulkan::InputAssemblyState& lhs, const portal::vulkan::InputAssemblyState& rhs)
{
	return std::tie(lhs.primitive_restart_enable, lhs.topology) != std::tie(rhs.primitive_restart_enable, rhs.topology);
}

bool operator!=(const portal::vulkan::RasterizationState& lhs, const portal::vulkan::RasterizationState& rhs)
{
	return std::tie(
			lhs.cull_mode,
			lhs.depth_bias_enable,
			lhs.depth_clamp_enable,
			lhs.front_face,
			lhs.front_face,
			lhs.polygon_mode,
			lhs.rasterizer_discard_enable
		) !=
		std::tie(
			rhs.cull_mode,
			rhs.depth_bias_enable,
			rhs.depth_clamp_enable,
			rhs.front_face,
			rhs.front_face,
			rhs.polygon_mode,
			rhs.rasterizer_discard_enable
		);
}

bool operator!=(const portal::vulkan::ViewportState& lhs, const portal::vulkan::ViewportState& rhs)
{
	return lhs.viewport_count != rhs.viewport_count || lhs.scissor_count != rhs.scissor_count;
}

bool operator!=(const portal::vulkan::MultisampleState& lhs, const portal::vulkan::MultisampleState& rhs)
{
	return std::tie(
			lhs.alpha_to_coverage_enable,
			lhs.alpha_to_one_enable,
			lhs.min_sample_shading,
			lhs.rasterization_samples,
			lhs.sample_mask,
			lhs.sample_shading_enable
		) !=
		std::tie(
			rhs.alpha_to_coverage_enable,
			rhs.alpha_to_one_enable,
			rhs.min_sample_shading,
			rhs.rasterization_samples,
			rhs.sample_mask,
			rhs.sample_shading_enable
		);
}

bool operator!=(const portal::vulkan::DepthStencilState& lhs, const portal::vulkan::DepthStencilState& rhs)
{
	return std::tie(lhs.depth_bounds_test_enable, lhs.depth_compare_op, lhs.depth_test_enable, lhs.depth_write_enable, lhs.stencil_test_enable) !=
		std::tie(rhs.depth_bounds_test_enable, rhs.depth_compare_op, rhs.depth_test_enable, rhs.depth_write_enable, rhs.stencil_test_enable) ||
		lhs.back != rhs.back || lhs.front != rhs.front;
}

bool operator!=(const portal::vulkan::ColorBlendState& lhs, const portal::vulkan::ColorBlendState& rhs)
{
	return std::tie(lhs.logic_op, lhs.logic_op_enable) != std::tie(rhs.logic_op, rhs.logic_op_enable) ||
		lhs.attachments.size() != rhs.attachments.size() ||
		!std::equal(
			lhs.attachments.begin(),
			lhs.attachments.end(),
			rhs.attachments.begin(),
			[](const portal::vulkan::ColorBlendAttachmentState& lhs, const portal::vulkan::ColorBlendAttachmentState& rhs)
			{
				return lhs == rhs;
			}
		);
}

namespace portal::vulkan
{
void VertexInputState::serialize(Serializer& ser) const
{
	ser << bindings << attributes;
}

VertexInputState VertexInputState::deserialize(Deserializer& des)
{
	return {
		.bindings = des.get_value<std::vector<vk::VertexInputBindingDescription>>(),
		.attributes = des.get_value<std::vector<vk::VertexInputAttributeDescription>>(),
	};
}

void InputAssemblyState::serialize(Serializer& ser) const
{
	ser << topology << primitive_restart_enable;
}

InputAssemblyState InputAssemblyState::deserialize(Deserializer& des)
{
	return {
		.topology = des.get_value<vk::PrimitiveTopology>(),
		.primitive_restart_enable = des.get_value<vk::Bool32>(),
	};
}

void RasterizationState::serialize(Serializer& ser) const
{
	ser << depth_clamp_enable << rasterizer_discard_enable << polygon_mode << cull_mode << front_face << depth_bias_enable;
}

RasterizationState RasterizationState::deserialize(Deserializer& des)
{
	return {
		.depth_clamp_enable = des.get_value<vk::Bool32>(),
		.rasterizer_discard_enable = des.get_value<vk::Bool32>(),
		.polygon_mode = des.get_value<vk::PolygonMode>(),
		.cull_mode = des.get_value<vk::CullModeFlagBits>(),
		.front_face = des.get_value<vk::FrontFace>(),
		.depth_bias_enable = des.get_value<vk::Bool32>()
	};
}

void ViewportState::serialize(Serializer& ser) const
{
	ser << viewport_count << scissor_count;
}

ViewportState ViewportState::deserialize(Deserializer& des)
{
	return {
		.viewport_count = des.get_value<uint32_t>(),
		.scissor_count = des.get_value<uint32_t>(),
	};
}

void MultisampleState::serialize(Serializer& ser) const
{
	ser << rasterization_samples << sample_shading_enable << min_sample_shading << sample_mask << alpha_to_coverage_enable << alpha_to_one_enable;
}

MultisampleState MultisampleState::deserialize(Deserializer& des)
{
	return {
		.rasterization_samples = des.get_value<vk::SampleCountFlagBits>(),
		.sample_shading_enable = des.get_value<vk::Bool32>(),
		.min_sample_shading = des.get_value<float>(),
		.sample_mask = des.get_value<vk::SampleMask>(),
		.alpha_to_coverage_enable = des.get_value<vk::Bool32>(),
		.alpha_to_one_enable = des.get_value<vk::Bool32>()
	};
}

void StencilOpState::serialize(Serializer& ser) const
{
	ser << fail_op << pass_op << depth_fail_op << compare_op;
}

StencilOpState StencilOpState::deserialize(Deserializer& des)
{
	return {
		.fail_op = des.get_value<vk::StencilOp>(),
		.pass_op = des.get_value<vk::StencilOp>(),
		.depth_fail_op = des.get_value<vk::StencilOp>(),
		.compare_op = des.get_value<vk::CompareOp>(),
	};
}

void DepthStencilState::serialize(Serializer& ser) const
{
	ser << depth_test_enable << depth_write_enable << depth_compare_op << depth_bounds_test_enable << stencil_test_enable << front << back;
}

DepthStencilState DepthStencilState::deserialize(Deserializer& des)
{
	return {
		.depth_test_enable = des.get_value<vk::Bool32>(),
		.depth_write_enable = des.get_value<vk::Bool32>(),
		.depth_compare_op = des.get_value<vk::CompareOp>(),
		.depth_bounds_test_enable = des.get_value<vk::Bool32>(),
		.stencil_test_enable = des.get_value<vk::Bool32>(),
		.front = des.get_value<StencilOpState>(),
		.back = des.get_value<StencilOpState>()
	};
}

vk::PipelineColorBlendAttachmentState ColorBlendAttachmentState::to_vk_attachment() const
{
	return {
		blend_enable,
		src_color_blend_factor,
		dst_color_blend_factor,
		color_blend_op,
		src_alpha_blend_factor,
		dst_alpha_blend_factor,
		alpha_blend_op,
		color_write_mask
	};
}

void ColorBlendAttachmentState::serialize(Serializer& ser) const
{
	ser << blend_enable << src_color_blend_factor << dst_color_blend_factor << color_blend_op << src_alpha_blend_factor << dst_alpha_blend_factor <<
		alpha_blend_op << static_cast<uint32_t>(color_write_mask);
}

ColorBlendAttachmentState ColorBlendAttachmentState::deserialize(Deserializer& des)
{
	return {
		.blend_enable = des.get_value<vk::Bool32>(),
		.src_color_blend_factor = des.get_value<vk::BlendFactor>(),
		.dst_color_blend_factor = des.get_value<vk::BlendFactor>(),
		.color_blend_op = des.get_value<vk::BlendOp>(),
		.src_alpha_blend_factor = des.get_value<vk::BlendFactor>(),
		.dst_alpha_blend_factor = des.get_value<vk::BlendFactor>(),
		.alpha_blend_op = des.get_value<vk::BlendOp>(),
		.color_write_mask = static_cast<vk::ColorComponentFlags>(des.get_value<uint32_t>())
	};
}

ColorBlendState ColorBlendState::deserialize(Deserializer& des)
{
	return {
		.logic_op_enable = des.get_value<vk::Bool32>(),
		.logic_op = des.get_value<vk::LogicOp>(),
		.attachments = des.get_value<std::vector<ColorBlendAttachmentState>>()
	};
}

void SpecializationConstantState::reset()
{
	if (dirty)
		specialization_constant_state.clear();

	dirty = false;
}

bool SpecializationConstantState::is_dirty() const
{
	return dirty;
}

void SpecializationConstantState::clear_dirty()
{
	dirty = false;
}

void SpecializationConstantState::set_constant(uint32_t constant_id, const std::vector<uint8_t>& value)
{
	const auto data = specialization_constant_state.find(constant_id);

	if (data != specialization_constant_state.end() && data->second == value)
		return;

	dirty = true;
	specialization_constant_state[constant_id] = value;
}

void SpecializationConstantState::set_specialization_constant_state(const std::map<uint32_t, std::vector<uint8_t>>& state)
{
	specialization_constant_state = state;
}

const std::map<uint32_t, std::vector<uint8_t>>& SpecializationConstantState::get_specialization_constant_state() const
{
	return specialization_constant_state;
}

void SpecializationConstantState::serialize(Serializer& serializer) const
{
	serializer << specialization_constant_state;
}

SpecializationConstantState SpecializationConstantState::deserialize(Deserializer& deserializer)
{
	SpecializationConstantState state{};
	deserializer.get_value(state.specialization_constant_state);
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PipelineState::reset()
{
	clear_dirty();
	pipeline_layout = nullptr;
	render_pass = nullptr;
	specialization_constant_state.reset();
	vertex_input_state = {};
	input_assembly_state = {};
	rasterization_state = {};
	multisample_state = {};
	depth_stencil_state = {};
	color_blend_state = {};
	subpass_index = {0U};
}

void PipelineState::set_pipeline_layout(PipelineLayout& new_pipeline_layout)
{
	if (pipeline_layout)
	{
		if (pipeline_layout->get_handle() != new_pipeline_layout.get_handle())
		{
			pipeline_layout = &new_pipeline_layout;
			dirty = true;
		}
	}
	else
	{
		pipeline_layout = &new_pipeline_layout;
		dirty = true;
	}
}

void PipelineState::set_render_pass(const RenderPass& new_render_pass)
{
	if (render_pass)
	{
		if (render_pass->get_handle() != new_render_pass.get_handle())
		{
			render_pass = &new_render_pass;
			dirty = true;
		}
	}
	else
	{
		render_pass = &new_render_pass;
		dirty = true;
	}
}

void PipelineState::set_specialization_constant(const uint32_t constant_id, const std::vector<uint8_t>& data)
{
	specialization_constant_state.set_constant(constant_id, data);
	if (specialization_constant_state.is_dirty())
		dirty = true;
}

void PipelineState::set_vertex_input_state(const VertexInputState& new_vertex_input_state)
{
	if (vertex_input_state != new_vertex_input_state)
	{
		vertex_input_state = new_vertex_input_state;
		dirty = true;
	}
}

void PipelineState::set_input_assembly_state(const InputAssemblyState& new_input_assembly_state)
{
	if (input_assembly_state != new_input_assembly_state)
	{
		input_assembly_state = new_input_assembly_state;
		dirty = true;
	}
}

void PipelineState::set_rasterization_state(const RasterizationState& new_rasterization_state)
{
	if (rasterization_state != new_rasterization_state)
	{
		rasterization_state = new_rasterization_state;
		dirty = true;
	}
}

void PipelineState::set_viewport_state(const ViewportState& new_viewport_state)
{
	if (viewport_state != new_viewport_state)
	{
		viewport_state = new_viewport_state;
		dirty = true;
	}
}

void PipelineState::set_multisample_state(const MultisampleState& new_multisample_state)
{
	if (multisample_state != new_multisample_state)
	{
		multisample_state = new_multisample_state;
		dirty = true;
	}
}

void PipelineState::set_depth_stencil_state(const DepthStencilState& new_depth_stencil_state)
{
	if (depth_stencil_state != new_depth_stencil_state)
	{
		depth_stencil_state = new_depth_stencil_state;
		dirty = true;
	}
}

void PipelineState::set_color_blend_state(const ColorBlendState& new_color_blend_state)
{
	if (color_blend_state != new_color_blend_state)
	{
		color_blend_state = new_color_blend_state;
		dirty = true;
	}
}

void PipelineState::set_subpass_index(const uint32_t new_subpass_index)
{
	if (subpass_index != new_subpass_index)
	{
		subpass_index = new_subpass_index;
		dirty = true;
	}
}

const PipelineLayout& PipelineState::get_pipeline_layout() const
{
	return *pipeline_layout;
}

const RenderPass* PipelineState::get_render_pass() const
{
	return render_pass;
}

const SpecializationConstantState& PipelineState::get_specialization_constant_state() const
{
	return specialization_constant_state;
}

const VertexInputState& PipelineState::get_vertex_input_state() const
{
	return vertex_input_state;
}

const InputAssemblyState& PipelineState::get_input_assembly_state() const
{
	return input_assembly_state;
}

const RasterizationState& PipelineState::get_rasterization_state() const
{
	return rasterization_state;
}

const ViewportState& PipelineState::get_viewport_state() const
{
	return viewport_state;
}

const MultisampleState& PipelineState::get_multisample_state() const
{
	return multisample_state;
}

const DepthStencilState& PipelineState::get_depth_stencil_state() const
{
	return depth_stencil_state;
}

const ColorBlendState& PipelineState::get_color_blend_state() const
{
	return color_blend_state;
}

uint32_t PipelineState::get_subpass_index() const
{
	return subpass_index;
}

bool PipelineState::is_dirty() const
{
	return dirty || specialization_constant_state.is_dirty();
}

void PipelineState::clear_dirty()
{
	dirty = false;
	specialization_constant_state.clear_dirty();
}

void PipelineState::serialize(Serializer& serializer) const
{
	serializer.add_value(get_subpass_index());
	serializer.add_value(get_specialization_constant_state());
	serializer.add_value(get_vertex_input_state());
	serializer.add_value(get_input_assembly_state());
	serializer.add_value(get_rasterization_state());
	serializer.add_value(get_viewport_state());
	serializer.add_value(get_multisample_state());
	serializer.add_value(get_depth_stencil_state());
	serializer.add_value(get_color_blend_state());
}

PipelineState PipelineState::deserialize(Deserializer& deserializer)
{
	PipelineState state{};
	deserializer.get_value(state.subpass_index);
	deserializer.get_value(state.specialization_constant_state);
	deserializer.get_value(state.vertex_input_state);
	deserializer.get_value(state.input_assembly_state);
	deserializer.get_value(state.rasterization_state);
	deserializer.get_value(state.viewport_state);
	deserializer.get_value(state.multisample_state);
	deserializer.get_value(state.depth_stencil_state);
	deserializer.get_value(state.color_blend_state);
	return state;
}
}
