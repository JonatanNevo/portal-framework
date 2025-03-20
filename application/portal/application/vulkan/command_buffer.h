//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once

#include "portal/application/vulkan/common.h"
#include "portal/application/vulkan/pipeline_states.h"
#include "portal/application/vulkan/base/vulkan_resource.h"
#include "portal/application/vulkan/resources/resource_binding_state.h"

namespace portal::vulkan
{

namespace rendering
{
	struct LightingState;
	class Subpass;
}

class QueryPool;

class DescriptorSetLayout;
class CommandPool;
class RenderTarget;
class PipelineLayout;
class Framebuffer;
class RenderPass;
class Sampler;
class Image;

class CommandBuffer final : public VulkanResource<vk::CommandBuffer>
{
public:
	struct RenderPassBinding
	{
		const RenderPass* render_pass;
		const Framebuffer* frame_buffer;
	};

	enum class ResetMode
	{
		ResetPool,
		ResetIndividually,
		AlwaysAllocate
	};

public:
	CommandBuffer(CommandPool& command_pool, vk::CommandBufferLevel level);
	CommandBuffer(CommandBuffer&& other) noexcept;
	~CommandBuffer() override;

	CommandBuffer(const CommandBuffer&) = delete;
	CommandBuffer& operator=(const CommandBuffer&) = delete;
	CommandBuffer& operator=(CommandBuffer&&) = delete;

	/**
	 * @brief Flushes the command buffer, pushing the new changes
	 * @param pipeline_bind_point The type of pipeline we want to flush
	 */
	void flush(vk::PipelineBindPoint pipeline_bind_point);

	/**
	 * @brief Sets the command buffer so that it is ready for recording
	 *        If it is a secondary command buffer, a pointer to the
	 *        primary command buffer it inherits from must be provided
	 * @param flags Usage behavior for the command buffer
	 * @param primary_cmd_buf (optional)
	 * @return Whether it succeeded or not
	 */
	vk::Result begin(vk::CommandBufferUsageFlags flags, CommandBuffer* primary_cmd_buf = nullptr);

	/**
	 * @brief Sets the command buffer so that it is ready for recording
	 *        If it is a secondary command buffer, pointers to the
	 *        render pass and framebuffer as well as subpasses index must be provided
	 * @param flags Usage behavior for the command buffer
	 * @param render_pass
	 * @param framebuffer
	 * @param subpass_index
	 * @return Whether it succeeded or not
	 */
	vk::Result begin(vk::CommandBufferUsageFlags flags, const RenderPass* render_pass, const Framebuffer* framebuffer, uint32_t subpass_index);

	vk::Result end();

	void clear(const vk::ClearAttachment& info, const vk::ClearRect& rect);

	void begin_render_pass(
		const RenderTarget& render_target,
		const std::vector<LoadStoreInfo>& load_store_infos,
		const std::vector<vk::ClearValue>& clear_values,
		const std::vector<std::unique_ptr<rendering::Subpass>>& subpasses,
		vk::SubpassContents contents = vk::SubpassContents::eInline
	);

	void begin_render_pass(
		const RenderTarget& render_target,
		const RenderPass& render_pass,
		const Framebuffer& framebuffer,
		const std::vector<vk::ClearValue>& clear_values,
		vk::SubpassContents contents = vk::SubpassContents::eInline
	);

	RenderPass& get_render_pass(
		const RenderTarget& render_target,
		const std::vector<LoadStoreInfo>& load_store_infos,
		const std::vector<std::unique_ptr<rendering::Subpass>>& subpasses
	);

	void end_render_pass();

	void next_subpass();

	void execute_commands(CommandBuffer& secondary_command_buffer);
	void execute_commands(std::vector<CommandBuffer*>& secondary_command_buffers);

	void bind_pipeline_layout(PipelineLayout& pipeline_layout);

	template <class T>
	void set_specialization_constant(uint32_t constant_id, const T& data);

	void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t>& data);

	/**
	 * @brief Records byte data into the command buffer to be pushed as push constants to each draw call
	 * @param values The byte data to store
	 */
	void push_constants(const std::vector<uint8_t>& values);

	template <typename T>
	void push_constants(const T& value)
	{
		auto data = to_bytes(value);

		uint32_t size = to_u32(stored_push_constants.size() + data.size());

		if (size > max_push_constants_size)
		{
			LOG_CORE_ERROR_TAG("Vulkan", "Push constant limit exceeded ({} / {} bytes)", size, max_push_constants_size);
			throw std::runtime_error("Cannot overflow push constant limit");
		}

		stored_push_constants.insert(stored_push_constants.end(), data.begin(), data.end());
	}

	void bind_buffer(const Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);
	void bind_image(const ImageView& image_view, const Sampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element);
	void bind_image(const ImageView& image_view, uint32_t set, uint32_t binding, uint32_t array_element);
	void bind_input(const ImageView& image_view, uint32_t set, uint32_t binding, uint32_t array_element);
	void bind_vertex_buffers(
		uint32_t first_binding,
		const std::vector<std::reference_wrapper<const Buffer>>& buffers,
		const std::vector<vk::DeviceSize>& offsets
	);
	void bind_index_buffer(const Buffer& buffer, vk::DeviceSize offset, vk::IndexType index_type);
	void bind_lighting(rendering::LightingState& lighting_state, uint32_t set, uint32_t binding);

	void set_viewport_state(const ViewportState& state_info);
	void set_vertex_input_state(const VertexInputState& state_info);
	void set_input_assembly_state(const InputAssemblyState& state_info);
	void set_rasterization_state(const RasterizationState& state_info);
	void set_multisample_state(const MultisampleState& state_info);
	void set_depth_stencil_state(const DepthStencilState& state_info);
	void set_color_blend_state(const ColorBlendState& state_info);
	void set_viewport(uint32_t first_viewport, const std::vector<vk::Viewport>& viewports);
	void set_scissor(uint32_t first_scissor, const std::vector<vk::Rect2D>& scissors);
	void set_line_width(float line_width);
	void set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);
	void set_blend_constants(const std::array<float, 4>& blend_constants);
	void set_depth_bounds(float min_depth_bounds, float max_depth_bounds);

	void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
	void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
	void draw_indexed_indirect(const Buffer& buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride);

	void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
	void dispatch_indirect(const Buffer& buffer, vk::DeviceSize offset);

	void update_buffer(const Buffer& buffer, vk::DeviceSize offset, const std::vector<uint8_t>& data);
	void copy_buffer(const Buffer& src_buffer, const Buffer& dst_buffer, vk::DeviceSize size);
	void buffer_memory_barrier(const Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize size, const BufferMemoryBarrier& memory_barrier);

	void blit_image(const Image& src_img, const Image& dst_img, const std::vector<vk::ImageBlit>& regions);
	void resolve_image(const Image& src_img, const Image& dst_img, const std::vector<vk::ImageResolve>& regions);
	void copy_image(const Image& src_img, const Image& dst_img, const std::vector<vk::ImageCopy>& regions);
	void image_memory_barrier(const ImageView& image_view, const ImageMemoryBarrier& memory_barrier) const;

	void copy_buffer_to_image(const Buffer& buffer, const Image& image, const std::vector<vk::BufferImageCopy>& regions);
	void copy_image_to_buffer(
		const Image& image,
		vk::ImageLayout image_layout,
		const Buffer& buffer,
		const std::vector<vk::BufferImageCopy>& regions
	);

	void set_update_after_bind(bool update_after_bind);

	void reset_query_pool(const QueryPool& query_pool, uint32_t first_query, uint32_t query_count);
	void begin_query(const QueryPool& query_pool, uint32_t query, vk::QueryControlFlags flags);
	void end_query(const QueryPool& query_pool, uint32_t query);

	void write_timestamp(vk::PipelineStageFlagBits pipeline_stage, const QueryPool& query_pool, uint32_t query);

	/**
	 * @brief Reset the command buffer to a state where it can be recorded to
	 * @param reset_mode How to reset the buffer, should match the one used by the pool to allocate it
	 */
	vk::Result reset(ResetMode reset_mode);


	const vk::CommandBufferLevel level;

private:
	[[nodiscard]] const RenderPassBinding& get_current_render_pass() const;
	uint32_t get_current_subpass_index() const;

	/**
	 * @brief Check that the render area is an optimal size by comparing to the render area granularity
	 */
	bool is_render_size_optimal(const vk::Extent2D& extent, const vk::Rect2D& render_area) const;

	/**
	 * @brief Flush the pipeline state
	 */
	void flush_pipeline_state(vk::PipelineBindPoint pipeline_bind_point);
	/**
	 * @brief Flush the descriptor set state
	 */
	void flush_descriptor_state(vk::PipelineBindPoint pipeline_bind_point);
	/**
	 * @brief Flush the push constant state
	 */
	void flush_push_constants();

	CommandPool& command_pool;
	RenderPassBinding current_render_pass;
	PipelineState pipeline_state;
	ResourceBindingState resource_binding_state;
	std::vector<uint8_t> stored_push_constants;
	uint32_t max_push_constants_size;
	vk::Extent2D last_framebuffer_extent{};
	vk::Extent2D last_render_area_extent{};

	// If true, it becomes the responsibility of the caller to update ANY descriptor bindings
	// that contain update after bind, as they won't be implicitly updated
	bool update_after_bind = false;

	std::unordered_map<uint32_t, const DescriptorSetLayout*> descriptor_set_layout_binding_state;
};

template <class T>
void CommandBuffer::set_specialization_constant(const uint32_t constant_id, const T& data)
{
	set_specialization_constant(constant_id, to_bytes(data));
}

template <>
inline void CommandBuffer::set_specialization_constant<bool>(const std::uint32_t constant_id, const bool& data)
{
	set_specialization_constant(constant_id, to_bytes(static_cast<uint32_t>(data)));
}
} // portal
