//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_hash.hpp>

#include "portal/application/application.h"
#include "portal/application/vulkan/descriptor_pool.h"
#include "portal/application/vulkan/descriptor_set_layout.h"
#include "portal/application/vulkan/image.h"
#include "portal/application/vulkan/image_view.h"
#include "portal/application/vulkan/pipeline_states.h"
#include "portal/application/vulkan/render_pass.h"
#include "portal/application/vulkan/shaders/shader_module.h"
#include "portal/application/vulkan/pipeline_layout.h"
#include "portal/application/vulkan/render_target.h"

namespace std
{
template <>
struct hash<portal::vulkan::ShaderSource>
{
	std::size_t operator()(const portal::vulkan::ShaderSource& shader_source) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, shader_source.get_id());
		return result;
	}
};

template <>
struct hash<portal::vulkan::ShaderVariant>
{
	std::size_t operator()(const portal::vulkan::ShaderVariant& shader_variant) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, shader_variant.get_id());
		return result;
	}
};

template <>
struct hash<portal::vulkan::ShaderModule>
{
	std::size_t operator()(const portal::vulkan::ShaderModule& shader_module) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, shader_module.get_id());
		return result;
	}
};

template <>
struct hash<portal::vulkan::DescriptorSetLayout>
{
	std::size_t operator()(const portal::vulkan::DescriptorSetLayout& descriptor_set_layout) const noexcept
	{
		std::size_t result = 0;

		portal::vulkan::hash_combine(result, descriptor_set_layout.get_handle());

		return result;
	}
};

template <>
struct hash<portal::vulkan::DescriptorPool>
{
	std::size_t operator()(const portal::vulkan::DescriptorPool& descriptor_pool) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, descriptor_pool.get_descriptor_set_layout());
		return result;
	}
};

template <>
struct hash<portal::vulkan::PipelineLayout>
{
	std::size_t operator()(const portal::vulkan::PipelineLayout& pipeline_layout) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, pipeline_layout.get_handle());
		return result;
	}
};

template <>
struct hash<portal::vulkan::RenderPass>
{
	std::size_t operator()(const portal::vulkan::RenderPass& render_pass) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, render_pass.get_handle());
		return result;
	}
};

template <>
struct hash<portal::vulkan::Attachment>
{
	std::size_t operator()(const portal::vulkan::Attachment& attachment) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::Format>>(attachment.format));
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::SampleCountFlagBits>>(attachment.samples));
		portal::vulkan::hash_combine(result, attachment.usage);
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::ImageLayout>>(attachment.initial_layout));
		return result;
	}
};

template <>
struct hash<portal::vulkan::LoadStoreInfo>
{
	std::size_t operator()(const portal::vulkan::LoadStoreInfo& load_store_info) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::AttachmentLoadOp>>(load_store_info.load_op));
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::AttachmentStoreOp>>(load_store_info.store_op));
		return result;
	}
};

template <>
struct hash<portal::vulkan::SubpassInfo>
{
	std::size_t operator()(const portal::vulkan::SubpassInfo& subpass_info) const noexcept
	{
		std::size_t result = 0;
		for (uint32_t output_attachment : subpass_info.output_attachments)
		{
			portal::vulkan::hash_combine(result, output_attachment);
		}
		for (uint32_t input_attachment : subpass_info.input_attachments)
		{
			portal::vulkan::hash_combine(result, input_attachment);
		}
		for (uint32_t resolve_attachment : subpass_info.color_resolve_attachments)
		{
			portal::vulkan::hash_combine(result, resolve_attachment);
		}
		portal::vulkan::hash_combine(result, subpass_info.disable_depth_stencil_attachment);
		portal::vulkan::hash_combine(result, subpass_info.depth_stencil_resolve_attachment);
		portal::vulkan::hash_combine(result, subpass_info.depth_stencil_resolve_mode);
		return result;
	}
};

template <>
struct hash<portal::vulkan::SpecializationConstantState>
{
	std::size_t operator()(const portal::vulkan::SpecializationConstantState& specialization_constant_state) const noexcept
	{
		std::size_t result = 0;
		for (auto constants : specialization_constant_state.get_specialization_constant_state())
		{
			portal::vulkan::hash_combine(result, constants.first);
			for (const auto data : constants.second)
			{
				portal::vulkan::hash_combine(result, data);
			}
		}
		return result;
	}
};

template <>
struct hash<portal::vulkan::ShaderResource>
{
	std::size_t operator()(const portal::vulkan::ShaderResource& shader_resource) const noexcept
	{
		std::size_t result = 0;
		if (shader_resource.type == portal::vulkan::ShaderResourceType::Input ||
			shader_resource.type == portal::vulkan::ShaderResourceType::Output ||
			shader_resource.type == portal::vulkan::ShaderResourceType::PushConstant ||
			shader_resource.type == portal::vulkan::ShaderResourceType::SpecializationConstant)
		{
			return result;
		}
		portal::vulkan::hash_combine(result, shader_resource.set);
		portal::vulkan::hash_combine(result, shader_resource.binding);
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<portal::vulkan::ShaderResourceType>>(shader_resource.type));
		portal::vulkan::hash_combine(result, shader_resource.mode);
		return result;
	}
};


template <>
struct hash<portal::vulkan::StencilOpState>
{
	std::size_t operator()(const portal::vulkan::StencilOpState& stencil) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::CompareOp>>(stencil.compare_op));
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::StencilOp>>(stencil.depth_fail_op));
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::StencilOp>>(stencil.fail_op));
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::StencilOp>>(stencil.pass_op));
		return result;
	}
};

template <>
struct hash<portal::vulkan::ColorBlendAttachmentState>
{
	std::size_t operator()(const portal::vulkan::ColorBlendAttachmentState& color_blend_attachment) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::BlendOp>>(color_blend_attachment.alpha_blend_op));
		portal::vulkan::hash_combine(result, color_blend_attachment.blend_enable);
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::BlendOp>>(color_blend_attachment.color_blend_op));
		portal::vulkan::hash_combine(result, color_blend_attachment.color_write_mask);
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::BlendFactor>>(color_blend_attachment.dst_alpha_blend_factor));
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::BlendFactor>>(color_blend_attachment.dst_color_blend_factor));
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::BlendFactor>>(color_blend_attachment.src_alpha_blend_factor));
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::BlendFactor>>(color_blend_attachment.src_color_blend_factor));
		return result;
	}
};

template <>
struct hash<portal::vulkan::RenderTarget>
{
	std::size_t operator()(const portal::vulkan::RenderTarget& render_target) const noexcept
	{
		std::size_t result = 0;

		for (auto& view : render_target.get_views())
		{
			portal::vulkan::hash_combine(result, view.get_handle());
			portal::vulkan::hash_combine(result, view.get_image().get_handle());
		}

		return result;
	}
};

template <>
struct hash<portal::vulkan::PipelineState>
{
	std::size_t operator()(const portal::vulkan::PipelineState& pipeline_state) const noexcept
	{
		std::size_t result = 0;
		portal::vulkan::hash_combine(result, pipeline_state.get_pipeline_layout().get_handle());
		// For graphics only
		if (auto render_pass = pipeline_state.get_render_pass())
		{
			portal::vulkan::hash_combine(result, render_pass->get_handle());
		}
		portal::vulkan::hash_combine(result, pipeline_state.get_specialization_constant_state());
		portal::vulkan::hash_combine(result, pipeline_state.get_subpass_index());
		for (auto shader_module : pipeline_state.get_pipeline_layout().get_shader_modules())
		{
			portal::vulkan::hash_combine(result, shader_module->get_id());
		}
		// vk::PipelineVertexInputStateCreateInfo
		for (auto& attribute : pipeline_state.get_vertex_input_state().attributes)
		{
			portal::vulkan::hash_combine(result, attribute);
		}
		for (auto& binding : pipeline_state.get_vertex_input_state().bindings)
		{
			portal::vulkan::hash_combine(result, binding);
		}
		// vk::PipelineInputAssemblyStateCreateInfo
		portal::vulkan::hash_combine(result, pipeline_state.get_input_assembly_state().primitive_restart_enable);
		portal::vulkan::hash_combine(
			result,
			static_cast<std::underlying_type_t<vk::PrimitiveTopology>>(pipeline_state.get_input_assembly_state().topology)
		);
		//vk::PipelineViewportStateCreateInfo
		portal::vulkan::hash_combine(result, pipeline_state.get_viewport_state().viewport_count);
		portal::vulkan::hash_combine(result, pipeline_state.get_viewport_state().scissor_count);
		// vk::PipelineRasterizationStateCreateInfo
		portal::vulkan::hash_combine(result, pipeline_state.get_rasterization_state().cull_mode);
		portal::vulkan::hash_combine(result, pipeline_state.get_rasterization_state().depth_bias_enable);
		portal::vulkan::hash_combine(result, pipeline_state.get_rasterization_state().depth_clamp_enable);
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type_t<vk::FrontFace>>(pipeline_state.get_rasterization_state().front_face));
		portal::vulkan::hash_combine(
			result,
			static_cast<std::underlying_type_t<vk::PolygonMode>>(pipeline_state.get_rasterization_state().polygon_mode)
		);
		portal::vulkan::hash_combine(result, pipeline_state.get_rasterization_state().rasterizer_discard_enable);
		// vk::PipelineMultisampleStateCreateInfo
		portal::vulkan::hash_combine(result, pipeline_state.get_multisample_state().alpha_to_coverage_enable);
		portal::vulkan::hash_combine(result, pipeline_state.get_multisample_state().alpha_to_one_enable);
		portal::vulkan::hash_combine(result, pipeline_state.get_multisample_state().min_sample_shading);
		portal::vulkan::hash_combine(
			result,
			static_cast<std::underlying_type_t<vk::SampleCountFlagBits>>(pipeline_state.get_multisample_state().rasterization_samples)
		);
		portal::vulkan::hash_combine(result, pipeline_state.get_multisample_state().sample_shading_enable);
		portal::vulkan::hash_combine(result, pipeline_state.get_multisample_state().sample_mask);
		// vk::PipelineDepthStencilStateCreateInfo
		portal::vulkan::hash_combine(result, pipeline_state.get_depth_stencil_state().back);
		portal::vulkan::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_bounds_test_enable);
		portal::vulkan::hash_combine(
			result,
			static_cast<std::underlying_type_t<vk::CompareOp>>(pipeline_state.get_depth_stencil_state().depth_compare_op)
		);
		portal::vulkan::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_test_enable);
		portal::vulkan::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_write_enable);
		portal::vulkan::hash_combine(result, pipeline_state.get_depth_stencil_state().front);
		portal::vulkan::hash_combine(result, pipeline_state.get_depth_stencil_state().stencil_test_enable);
		// vk::PipelineColorBlendStateCreateInfo
		portal::vulkan::hash_combine(result, static_cast<std::underlying_type<vk::LogicOp>::type>(pipeline_state.get_color_blend_state().logic_op));
		portal::vulkan::hash_combine(result, pipeline_state.get_color_blend_state().logic_op_enable);
		for (auto& attachment : pipeline_state.get_color_blend_state().attachments)
		{
			portal::vulkan::hash_combine(result, attachment);
		}
		return result;
	}
};
}

namespace portal::vulkan
{
template <typename T>
concept HasId = requires(T t) {
	{ t.get_id() } -> std::convertible_to<size_t>;
};

template <typename T>
void hash_param(size_t& seed, const T& value)
{
	hash_combine(seed, value);
}

template <>
inline void hash_param(size_t& /*seed*/, const VkPipelineCache& /*value*/)
{
}

template <>
inline void hash_param<std::vector<uint8_t>>(size_t& seed, const std::vector<uint8_t>& value)
{
	hash_combine(seed, std::string{value.begin(), value.end()});
}

template <HasId T>
void hash_param(size_t& seed, const std::vector<T*>& value)
{
	for (auto& v : value)
	{
		hash_combine(seed, v->get_id());
	}
}

template <typename T> requires !HasId<T>
void hash_param(size_t& seed, const std::vector<T>& value)
{
	for (auto& v : value)
	{
		hash_combine(seed, v);
	}
}

template <typename T>
void hash_param(
	size_t& seed,
	const std::map<uint32_t, std::map<uint32_t, T>>& value
)
{
	for (const auto& binding_set : value)
	{
		hash_combine(seed, binding_set.first);

		for (const auto& binding_element : binding_set.second)
		{
			hash_combine(seed, binding_element.first);
			hash_combine(seed, binding_element.second);
		}
	}
}

template <typename T, typename... Args>
void hash_param(size_t& seed, const T& first_arg, const Args&... args)
{
	hash_param(seed, first_arg);
	hash_param(seed, args...);
}


template <class T, class... A>
T& request_resource(Device& device, std::unordered_map<std::size_t, T>& resources, A&... args)
{
	std::unordered_map<const T*, size_t> index_mapping;
	return request_resource(device, resources, index_mapping, args...);
}

template <class T, class... A>
T& request_resource(Device& device, std::unordered_map<std::size_t, T>& resources, std::unordered_map<const T*, size_t>& index_mapping, A&... args)
{
	std::size_t hash = 0;
	hash_param(hash, args...);

	auto resource_it = resources.find(hash);
	if (resource_it != resources.end())
		return resource_it->second;

	// If we do not have it already, create and cache it
	const char* res_type = typeid(T).name();
	size_t res_id = resources.size();

	LOG_CORE_DEBUG_TAG("Vulkan", "Building #{} cache object ({})", res_id, res_type);

	// Only error handle in release
#ifndef PORTAL_DEBUG
	try
	{
#endif
	T resource(device, args...);
	auto res_ins_it = resources.emplace(hash, std::move(resource));

	if (!res_ins_it.second)
		throw std::runtime_error{std::string{"Insertion error for #"} + std::to_string(res_id) + "cache object (" + res_type + ")"};

	resource_it = res_ins_it.first;
	// Fix: Store the pointer to the actual object and its index
	index_mapping.emplace(&resource_it->second, res_id);
#ifndef PORTAL_DEBUG
	}
	catch (const std::exception &e)
	{
		LOG_CORE_ERROR_TAG("Vulkan", "Creation error for #{} cache object ({})", res_id, res_type);
		throw e;
	}
#endif

	return resource_it->second;
}
}
