//
// Created by Jonatan Nevo on 07/03/2025.
//

#pragma once

#include <portal/core/glm.h>

#include "portal/application/vulkan/base/buffer_pool.h"
#include "portal/application/vulkan/shaders/shader_module.h"
#include "portal/application/vulkan/rendering/render_context.h"
#include "portal/application/vulkan/rendering/render_frame.h"

namespace portal::vulkan::rendering
{
class RenderContext;

struct alignas(16) Light
{
	glm::vec4 position;  // position.w represents type of light
	glm::vec4 color;     // color.w represents light intensity
	glm::vec4 direction; // direction.w represents range
	glm::vec2 info;      // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

struct LightingState
{
	std::vector<Light> directional_lights;
	std::vector<Light> point_lights;
	std::vector<Light> spot_lights;
	BufferAllocation light_buffer;
};

enum class LightType
{
	Directional = 0,
	Point       = 1,
	Spot        = 2,
	Max
};

/**
 * @brief Calculates the vulkan style projection matrix
 * @param proj The projection matrix
 * @return The vulkan style projection matrix
 */
glm::mat4 vulkan_style_projection(const glm::mat4& proj);

/**
 * @brief This class defines an interface for subpasses
 *        where they need to implement the draw function.
 *        It is used to construct a RenderPipeline
 */
class Subpass
{
public:
	Subpass(RenderContext& render_context, ShaderSource&& vertex_shader, ShaderSource&& fragment_shader);
	Subpass(const Subpass&) = delete;
	Subpass(Subpass&&) = default;
	virtual ~Subpass() = default;
	Subpass& operator=(const Subpass&) = delete;
	Subpass& operator=(Subpass&&) = delete;

	/**
	 * @brief Draw virtual function
	 * @param command_buffer Command buffer to use to record draw commands
	 */
	virtual void draw(vulkan::CommandBuffer& command_buffer) = 0;

	/**
	 * @brief Prepares the shaders and shader variants for a subpass
	 */
	virtual void prepare() = 0;

	/**
	 * @brief Prepares the lighting state to have its lights
	 *
	 * @tparam A light structure that has 'directional_lights', 'point_lights' and 'spot_light' array fields defined.
	 * @param scene_lights All the light components from the scene graph
	 * @param max_lights_per_type The maximum amount of lights allowed for any given type of light.
	 */
	template <typename T>
	void allocate_lights(const std::unordered_map<LightType, std::vector<Light>>& scene_lights, size_t max_lights_per_type);

	const std::vector<uint32_t>& get_color_resolve_attachments() const;
	const std::string& get_debug_name() const;
	const uint32_t& get_depth_stencil_resolve_attachment() const;
	vk::ResolveModeFlagBits get_depth_stencil_resolve_mode() const;
	DepthStencilState& get_depth_stencil_state();
	const bool& get_disable_depth_stencil_attachment() const;
	const ShaderSource& get_fragment_shader() const;
	const std::vector<uint32_t>& get_input_attachments() const;
	LightingState& get_lighting_state();
	const std::vector<uint32_t>& get_output_attachments() const;
	RenderContext& get_render_context() const;
	std::unordered_map<std::string, ShaderResourceMode> const& get_resource_mode_map() const;
	vk::SampleCountFlagBits get_sample_count() const;
	const ShaderSource& get_vertex_shader() const;

	void set_color_resolve_attachments(std::vector<uint32_t> const& color_resolve);
	void set_debug_name(const std::string& name);
	void set_disable_depth_stencil_attachment(bool disable_depth_stencil);
	void set_depth_stencil_resolve_attachment(uint32_t depth_stencil_resolve);
	void set_depth_stencil_resolve_mode(vk::ResolveModeFlagBits mode);
	void set_input_attachments(std::vector<uint32_t> const& input);
	void set_output_attachments(std::vector<uint32_t> const& output);
	void set_sample_count(vk::SampleCountFlagBits sample_count);

	/**
	 * @brief Updates the render target attachments with the ones stored in this subpass
	 *        This function is called by the RenderPipeline before beginning the render
	 *        pass and before proceeding with a new subpass.
	 */
	void update_render_target_attachments(RenderTarget& render_target) const;

private:
	/// Default to no color resolve attachments
	std::vector<uint32_t> color_resolve_attachments = {};
	std::string debug_name{};
	/**
	 * @brief When creating the renderpass, if not None, the resolve
	 *        of the multisampled depth attachment will be enabled,
	 *        with this mode, to depth_stencil_resolve_attachment
	 */
	vk::ResolveModeFlagBits depth_stencil_resolve_mode{vk::ResolveModeFlagBits::eNone};
	DepthStencilState depth_stencil_state{};
	/**
	 * @brief When creating the renderpass, pDepthStencilAttachment will
	 *        be set to nullptr, which disables depth testing
	 */
	bool disable_depth_stencil_attachment{false};
	/// Default to no depth stencil resolve attachment
	uint32_t depth_stencil_resolve_attachment{VK_ATTACHMENT_UNUSED};
	/// The structure containing all the requested render-ready lights for the scene
	LightingState lighting_state{};
	ShaderSource fragment_shader;
	/// Default to no input attachments
	std::vector<uint32_t> input_attachments = {};
	/// Default to swapchain output attachment
	std::vector<uint32_t> output_attachments = {0};
	RenderContext& render_context;
	// A map of shader resource names and the mode of constant data
	std::unordered_map<std::string, ShaderResourceMode> resource_mode_map;
	vk::SampleCountFlagBits sample_count{vk::SampleCountFlagBits::e1};
	ShaderSource vertex_shader;
};

template <typename T>
void Subpass::allocate_lights(const std::unordered_map<LightType, std::vector<Light>>& scene_lights, size_t max_lights_per_type)
{
	for (auto& [light_type, lights] : scene_lights)
	{
		for (auto& light : lights)
		{
			switch (light_type)
			{
			case LightType::Directional:
				{
					if (lighting_state.directional_lights.size() < max_lights_per_type)
						lighting_state.directional_lights.push_back(light);
					else
						LOG_CORE_ERROR_TAG(
						"Vulkan",
						"Subpass::allocate_lights: exceeding max_lights_per_type of {} for directional lights",
						max_lights_per_type
					);
					break;
				}
			case LightType::Point:
				{
					if (lighting_state.point_lights.size() < max_lights_per_type)
						lighting_state.point_lights.push_back(light);
					else
						LOG_CORE_ERROR_TAG(
						"Vulkan",
						"Subpass::allocate_lights: exceeding max_lights_per_type of {} for point lights",
						max_lights_per_type
					);
				}
			case LightType::Spot:
				{
					if (lighting_state.spot_lights.size() < max_lights_per_type)
						lighting_state.spot_lights.push_back(light);
					else
						LOG_CORE_ERROR_TAG(
						"Vulkan",
						"Subpass::allocate_lights: exceeding max_lights_per_type of {} for spot lights",
						max_lights_per_type
					);
				}
			default:
				LOG_CORE_ERROR_TAG("Vulkan", "Subpass::allocate_lights: encountered unknown light type");
				break;
			}
		}
	}

	T light_info;

	std::copy(lighting_state.directional_lights.begin(), lighting_state.directional_lights.end(), light_info.directional_lights);
	std::copy(lighting_state.point_lights.begin(), lighting_state.point_lights.end(), light_info.point_lights);
	std::copy(lighting_state.spot_lights.begin(), lighting_state.spot_lights.end(), light_info.spot_lights);

	auto& render_frame = render_context.get_active_frame();
	lighting_state.light_buffer = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(T));
	lighting_state.light_buffer.update(light_info);
}
} // portal
