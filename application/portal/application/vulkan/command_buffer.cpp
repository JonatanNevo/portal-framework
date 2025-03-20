//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "command_buffer.h"

#include <ranges>

#include "portal/application/vulkan/command_pool.h"
#include "portal/application/vulkan/descriptor_set_layout.h"
#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/frame_buffer.h"
#include "portal/application/vulkan/pipeline.h"
#include "portal/application/vulkan/pipeline_layout.h"
#include "portal/application/vulkan/query_pool.h"
#include "portal/application/vulkan/render_pass.h"
#include "portal/application/vulkan/render_target.h"
#include "portal/application/vulkan/rendering/subpass.h"

namespace portal::vulkan
{
CommandBuffer::CommandBuffer(CommandPool& command_pool, vk::CommandBufferLevel level)
    : VulkanResource(nullptr, &command_pool.get_device()),
      level(level),
      command_pool(command_pool),
      max_push_constants_size(get_device().get_gpu().get_properties().limits.maxPushConstantsSize)
{
    const vk::CommandBufferAllocateInfo allocate_info(command_pool.get_handle(), level, 1);
    set_handle(get_device_handle().allocateCommandBuffers(allocate_info).front());
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
    : VulkanResource(std::move(other)),
      level(other.level),
      command_pool(other.command_pool),
      current_render_pass(std::exchange(other.current_render_pass, {})),
      pipeline_state(std::exchange(other.pipeline_state, {})),
      resource_binding_state(std::exchange(other.resource_binding_state, {})),
      stored_push_constants(std::exchange(other.stored_push_constants, {})),
      max_push_constants_size(std::exchange(other.max_push_constants_size, {})),
      last_framebuffer_extent(std::exchange(other.last_framebuffer_extent, {})),
      last_render_area_extent(std::exchange(other.last_render_area_extent, {})),
      update_after_bind(std::exchange(other.update_after_bind, {})),
      descriptor_set_layout_binding_state(std::exchange(other.descriptor_set_layout_binding_state, {}))
{}

CommandBuffer::~CommandBuffer()
{
    if (has_handle())
        get_device_handle().freeCommandBuffers(command_pool.get_handle(), get_handle());
}

void CommandBuffer::flush(const vk::PipelineBindPoint pipeline_bind_point)
{
    flush_pipeline_state(pipeline_bind_point);
    flush_push_constants();
    flush_descriptor_state(pipeline_bind_point);
}

vk::Result CommandBuffer::begin(vk::CommandBufferUsageFlags flags, CommandBuffer* primary_cmd_buf)
{
    if (level == vk::CommandBufferLevel::eSecondary)
    {
        PORTAL_CORE_ASSERT(primary_cmd_buf, "Secondary command buffer must have a primary command buffer");
        const auto& [render_pass, frame_buffer] = primary_cmd_buf->get_current_render_pass();

        return begin(flags, render_pass, frame_buffer, primary_cmd_buf->get_current_subpass_index());
    }

    return begin(flags, nullptr, nullptr, 0);
}

vk::Result CommandBuffer::begin(
    const vk::CommandBufferUsageFlags flags,
    const RenderPass* render_pass,
    const Framebuffer* framebuffer,
    const uint32_t subpass_index
)
{
    // Reset state
    pipeline_state.reset();
    resource_binding_state.reset();
    descriptor_set_layout_binding_state.clear();
    stored_push_constants.clear();

    vk::CommandBufferBeginInfo begin_info(flags);
    vk::CommandBufferInheritanceInfo inheritance{};

    if (level == vk::CommandBufferLevel::eSecondary)
    {
        PORTAL_CORE_ASSERT(render_pass && framebuffer, "Render pass and framebuffer must be provided for secondary command buffer");
        current_render_pass.render_pass = render_pass;
        current_render_pass.frame_buffer = framebuffer;

        inheritance.renderPass = render_pass->get_handle();
        inheritance.framebuffer = framebuffer->get_handle();
        inheritance.subpass = subpass_index;

        begin_info.pInheritanceInfo = &inheritance;
    }

    get_handle().begin(begin_info);
    return vk::Result::eSuccess;
}

vk::Result CommandBuffer::end()
{
    get_handle().end();

    return vk::Result::eSuccess;
}

void CommandBuffer::clear(const vk::ClearAttachment& info, const vk::ClearRect& rect)
{
    get_handle().clearAttachments(info, rect);
}

void CommandBuffer::begin_render_pass(
    const RenderTarget& render_target,
    const std::vector<LoadStoreInfo>& load_store_infos,
    const std::vector<vk::ClearValue>& clear_values,
    const std::vector<std::unique_ptr<rendering::Subpass>>& subpasses,
    const vk::SubpassContents contents
)
{
    // Reset state
    pipeline_state.reset();
    resource_binding_state.reset();
    descriptor_set_layout_binding_state.clear();

    const auto& render_pass = get_render_pass(render_target, load_store_infos, subpasses);
    const auto& framebuffer = get_device().get_resource_cache().request_framebuffer(render_target, render_pass);

    begin_render_pass(render_target, render_pass, framebuffer, clear_values, contents);
}

void CommandBuffer::begin_render_pass(
    const RenderTarget& render_target,
    const RenderPass& render_pass,
    const Framebuffer& framebuffer,
    const std::vector<vk::ClearValue>& clear_values,
    vk::SubpassContents contents
)
{
    current_render_pass.render_pass = &render_pass;
    current_render_pass.frame_buffer = &framebuffer;

    // Begin render pass
    vk::RenderPassBeginInfo begin_info(
        current_render_pass.render_pass->get_handle(),
        current_render_pass.frame_buffer->get_handle(),
        {{}, render_target.get_extent()},
        clear_values
    );

    const auto& framebuffer_extent = current_render_pass.frame_buffer->get_extent();

    // Test the requested render area to confirm that it is optimal and could not cause a performance reduction
    if (!is_render_size_optimal(framebuffer_extent, begin_info.renderArea))
    {
        // Only prints the warning if the framebuffer or render area are different since the last time the render size was not optimal
        if ((framebuffer_extent != last_framebuffer_extent) || (begin_info.renderArea.extent != last_render_area_extent))
            LOG_CORE_WARN_TAG("Vulkan", "Render target extent is not an optimal size, this may result in reduced performance.");

        last_framebuffer_extent = framebuffer_extent;
        last_render_area_extent = begin_info.renderArea.extent;
    }

    get_handle().beginRenderPass(begin_info, contents);

    // Update blend state attachments for first subpasses
    auto blend_state = pipeline_state.get_color_blend_state();
    blend_state.attachments.resize(current_render_pass.render_pass->get_color_output_count(pipeline_state.get_subpass_index()));
    pipeline_state.set_color_blend_state(blend_state);
}

RenderPass& CommandBuffer::get_render_pass(
    const RenderTarget& render_target,
    const std::vector<LoadStoreInfo>& load_store_infos,
    const std::vector<std::unique_ptr<rendering::Subpass>>& subpasses
)
{
    PORTAL_CORE_ASSERT(subpasses.size() > 0, "Subpass count must be greater than zero");
    std::vector<SubpassInfo> subpass_infos(subpasses.size());

    auto subpass_info_it = subpass_infos.begin();
    for (const auto& subpass : subpasses)
    {
        subpass_info_it->input_attachments = subpass->get_input_attachments();
        subpass_info_it->output_attachments = subpass->get_output_attachments();
        subpass_info_it->color_resolve_attachments = subpass->get_color_resolve_attachments();
        subpass_info_it->disable_depth_stencil_attachment = subpass->get_disable_depth_stencil_attachment();
        subpass_info_it->depth_stencil_resolve_mode = subpass->get_depth_stencil_resolve_mode();
        subpass_info_it->depth_stencil_resolve_attachment = subpass->get_depth_stencil_resolve_attachment();
        subpass_info_it->debug_name = subpass->get_debug_name();

        ++subpass_info_it;
    }
    return get_device().get_resource_cache().request_render_pass(render_target.get_attachments(), load_store_infos, subpass_infos);
}

void CommandBuffer::end_render_pass()
{
    get_handle().endRenderPass();
}

void CommandBuffer::next_subpass()
{
    // Increment subpass index
    pipeline_state.set_subpass_index(pipeline_state.get_subpass_index() + 1);

    // Update blend state attachments
    auto blend_state = pipeline_state.get_color_blend_state();
    blend_state.attachments.resize(current_render_pass.render_pass->get_color_output_count(pipeline_state.get_subpass_index()));
    pipeline_state.set_color_blend_state(blend_state);

    // Reset descriptor sets
    resource_binding_state.reset();
    descriptor_set_layout_binding_state.clear();

    // Clear stored push constants
    stored_push_constants.clear();

    get_handle().nextSubpass(vk::SubpassContents::eInline);
}

void CommandBuffer::execute_commands(CommandBuffer& secondary_command_buffer)
{
    get_handle().executeCommands(secondary_command_buffer.get_handle());
}

void CommandBuffer::execute_commands(std::vector<CommandBuffer*>& secondary_command_buffers)
{
    std::vector<vk::CommandBuffer> sec_cmd_buf_handles(secondary_command_buffers.size(), nullptr);
    std::ranges::transform(
        secondary_command_buffers,
        sec_cmd_buf_handles.begin(),
        [](const CommandBuffer* sec_cmd_buf) { return sec_cmd_buf->get_handle(); }
    );
    get_handle().executeCommands(sec_cmd_buf_handles);
}

void CommandBuffer::bind_pipeline_layout(PipelineLayout& pipeline_layout)
{
    pipeline_state.set_pipeline_layout(pipeline_layout);
}

void CommandBuffer::set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t>& data)
{
    pipeline_state.set_specialization_constant(constant_id, data);
}

void CommandBuffer::push_constants(const std::vector<uint8_t>& values)
{
    uint32_t push_constant_size = static_cast<uint32_t>(stored_push_constants.size() + values.size());

    if (push_constant_size > max_push_constants_size)
    {
        LOG_CORE_ERROR_TAG(
            "Vulkan",
            "Push constant limit of {} exceeded (pushing {} bytes for a total of {} bytes)",
            max_push_constants_size,
            values.size(),
            push_constant_size
        );
        throw std::runtime_error("Push constant limit exceeded.");
    }
    stored_push_constants.insert(stored_push_constants.end(), values.begin(), values.end());
}

void CommandBuffer::bind_buffer(
    const Buffer& buffer,
    vk::DeviceSize offset,
    vk::DeviceSize range,
    uint32_t set,
    uint32_t binding,
    uint32_t array_element
)
{
    resource_binding_state.bind_buffer(buffer, offset, range, set, binding, array_element);
}

void CommandBuffer::bind_image(const ImageView& image_view, const Sampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element)
{
    resource_binding_state.bind_image(image_view, sampler, set, binding, array_element);
}

void CommandBuffer::bind_image(const ImageView& image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
    resource_binding_state.bind_image(image_view, set, binding, array_element);
}

void CommandBuffer::bind_input(const ImageView& image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
    resource_binding_state.bind_input(image_view, set, binding, array_element);
}

void CommandBuffer::bind_vertex_buffers(
    uint32_t first_binding,
    const std::vector<std::reference_wrapper<const Buffer>>& buffers,
    const std::vector<vk::DeviceSize>& offsets
)
{
    std::vector<vk::Buffer> buffer_handles(buffers.size(), nullptr);
    std::ranges::transform(buffers, buffer_handles.begin(), [](const Buffer& buffer) { return buffer.get_handle(); });
    get_handle().bindVertexBuffers(first_binding, buffer_handles, offsets);
}

void CommandBuffer::bind_index_buffer(const Buffer& buffer, vk::DeviceSize offset, vk::IndexType index_type)
{
    get_handle().bindIndexBuffer(buffer.get_handle(), offset, index_type);
}

void CommandBuffer::bind_lighting(rendering::LightingState& lighting_state, const uint32_t set, const uint32_t binding)
{
    bind_buffer(
        lighting_state.light_buffer.get_buffer(),
        lighting_state.light_buffer.get_offset(),
        lighting_state.light_buffer.get_size(),
        set,
        binding,
        0
    );

    set_specialization_constant(0, lighting_state.directional_lights.size());
    set_specialization_constant(1, lighting_state.point_lights.size());
    set_specialization_constant(2, lighting_state.spot_lights.size());
}

void CommandBuffer::set_viewport_state(const ViewportState& state_info)
{
    pipeline_state.set_viewport_state(state_info);
}

void CommandBuffer::set_vertex_input_state(const VertexInputState& state_info)
{
    pipeline_state.set_vertex_input_state(state_info);
}

void CommandBuffer::set_input_assembly_state(const InputAssemblyState& state_info)
{
    pipeline_state.set_input_assembly_state(state_info);
}

void CommandBuffer::set_rasterization_state(const RasterizationState& state_info)
{
    pipeline_state.set_rasterization_state(state_info);
}

void CommandBuffer::set_multisample_state(const MultisampleState& state_info)
{
    pipeline_state.set_multisample_state(state_info);
}

void CommandBuffer::set_depth_stencil_state(const DepthStencilState& state_info)
{
    pipeline_state.set_depth_stencil_state(state_info);
}

void CommandBuffer::set_color_blend_state(const ColorBlendState& state_info)
{
    pipeline_state.set_color_blend_state(state_info);
}

void CommandBuffer::set_viewport(const uint32_t first_viewport, const std::vector<vk::Viewport>& viewports)
{
    get_handle().setViewport(first_viewport, viewports);
}

void CommandBuffer::set_scissor(const uint32_t first_scissor, const std::vector<vk::Rect2D>& scissors)
{
    get_handle().setScissor(first_scissor, scissors);
}

void CommandBuffer::set_line_width(const float line_width)
{
    get_handle().setLineWidth(line_width);
}

void CommandBuffer::set_depth_bias(const float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor)
{
    get_handle().setDepthBias(depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
}

void CommandBuffer::set_blend_constants(const std::array<float, 4>& blend_constants)
{
    get_handle().setBlendConstants(blend_constants.data());
}

void CommandBuffer::set_depth_bounds(const float min_depth_bounds, float max_depth_bounds)
{
    get_handle().setDepthBounds(min_depth_bounds, max_depth_bounds);
}

void CommandBuffer::draw(const uint32_t vertex_count, const uint32_t instance_count, const uint32_t first_vertex, const uint32_t first_instance)
{
    flush(vk::PipelineBindPoint::eGraphics);
    get_handle().draw(vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::draw_indexed(
    const uint32_t index_count,
    const unsigned int instance_count,
    const uint32_t first_index,
    const int32_t vertex_offset,
    const uint32_t first_instance
)
{
    flush(vk::PipelineBindPoint::eGraphics);
    get_handle().drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::draw_indexed_indirect(const Buffer& buffer, const vk::DeviceSize offset, const uint32_t draw_count, const uint32_t stride)
{
    flush(vk::PipelineBindPoint::eGraphics);
    get_handle().drawIndexedIndirect(buffer.get_handle(), offset, draw_count, stride);
}

void CommandBuffer::dispatch(const uint32_t group_count_x, const uint32_t group_count_y, const uint32_t group_count_z)
{
    flush(vk::PipelineBindPoint::eCompute);
    get_handle().dispatch(group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::dispatch_indirect(const Buffer& buffer, const vk::DeviceSize offset)
{
    flush(vk::PipelineBindPoint::eCompute);
    get_handle().dispatchIndirect(buffer.get_handle(), offset);
}

void CommandBuffer::update_buffer(const Buffer& buffer, const vk::DeviceSize offset, const std::vector<uint8_t>& data)
{
    get_handle().updateBuffer<uint8_t>(buffer.get_handle(), offset, data);
}

void CommandBuffer::copy_buffer(const Buffer& src_buffer, const Buffer& dst_buffer, vk::DeviceSize size)
{
    const vk::BufferCopy copy_region({}, {}, size);
    get_handle().copyBuffer(src_buffer.get_handle(), dst_buffer.get_handle(), copy_region);
}

void CommandBuffer::buffer_memory_barrier(
    const Buffer& buffer,
    const vk::DeviceSize offset,
    const vk::DeviceSize size,
    const BufferMemoryBarrier& memory_barrier
)
{
    const vk::BufferMemoryBarrier buffer_memory_barrier(
        memory_barrier.src_access_mask,
        memory_barrier.dst_access_mask,
        {},
        {},
        buffer.get_handle(),
        offset,
        size
    );
    const vk::PipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
    const vk::PipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

    get_handle().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, buffer_memory_barrier, {});
}

void CommandBuffer::blit_image(const Image& src_img, const Image& dst_img, const std::vector<vk::ImageBlit>& regions)
{
    get_handle().blitImage(
        src_img.get_handle(),
        vk::ImageLayout::eTransferSrcOptimal,
        dst_img.get_handle(),
        vk::ImageLayout::eTransferDstOptimal,
        regions,
        vk::Filter::eNearest
    );
}

void CommandBuffer::resolve_image(const Image& src_img, const Image& dst_img, const std::vector<vk::ImageResolve>& regions)
{
    get_handle().resolveImage(
        src_img.get_handle(),
        vk::ImageLayout::eTransferSrcOptimal,
        dst_img.get_handle(),
        vk::ImageLayout::eTransferDstOptimal,
        regions
    );
}

void CommandBuffer::copy_image(const Image& src_img, const Image& dst_img, const std::vector<vk::ImageCopy>& regions)
{
    get_handle().copyImage(
        src_img.get_handle(),
        vk::ImageLayout::eTransferSrcOptimal,
        dst_img.get_handle(),
        vk::ImageLayout::eTransferDstOptimal,
        regions
    );
}

void CommandBuffer::image_memory_barrier(const ImageView& image_view, const ImageMemoryBarrier& memory_barrier) const
{
    // Adjust barrier's subresource range for depth images
    auto subresource_range = image_view.get_subresource_range();
    const auto format = image_view.get_format();
    if (is_depth_only_format(format))
    {
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
    else if (is_depth_stencil_format(format))
    {
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    }

    const vk::ImageMemoryBarrier image_memory_barrier(
        memory_barrier.src_access_mask,
        memory_barrier.dst_access_mask,
        memory_barrier.old_layout,
        memory_barrier.new_layout,
        memory_barrier.old_queue_family,
        memory_barrier.new_queue_family,
        image_view.get_image().get_handle(),
        subresource_range
    );

    const vk::PipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
    const vk::PipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;
    get_handle().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, image_memory_barrier);
}

void CommandBuffer::copy_buffer_to_image(const Buffer& buffer, const Image& image, const std::vector<vk::BufferImageCopy>& regions)
{
    get_handle().copyBufferToImage(buffer.get_handle(), image.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
}

void CommandBuffer::copy_image_to_buffer(
    const Image& image,
    const vk::ImageLayout image_layout,
    const Buffer& buffer,
    const std::vector<vk::BufferImageCopy>& regions
)
{
    get_handle().copyImageToBuffer(image.get_handle(), image_layout, buffer.get_handle(), regions);
}

void CommandBuffer::set_update_after_bind(bool update_after_bind)
{
    this->update_after_bind = update_after_bind;
}

void CommandBuffer::reset_query_pool(const QueryPool& query_pool, uint32_t first_query, uint32_t query_count)
{
    get_handle().resetQueryPool(query_pool.get_handle(), first_query, query_count);
}

void CommandBuffer::begin_query(const QueryPool& query_pool, uint32_t query, vk::QueryControlFlags flags)
{
    get_handle().beginQuery(query_pool.get_handle(), query, flags);
}

void CommandBuffer::end_query(const QueryPool& query_pool, uint32_t query)
{
    get_handle().endQuery(query_pool.get_handle(), query);
}

void CommandBuffer::write_timestamp(vk::PipelineStageFlagBits pipeline_stage, const QueryPool& query_pool, uint32_t query)
{
    get_handle().writeTimestamp(pipeline_stage, query_pool.get_handle(), query);
}

vk::Result CommandBuffer::reset(const ResetMode reset_mode)
{
    PORTAL_CORE_ASSERT(reset_mode == command_pool.get_reset_mode(), "Command buffer reset mode must match the one used by the pool to allocate it");

    if (reset_mode == ResetMode::ResetIndividually)
        get_handle().reset(vk::CommandBufferResetFlagBits::eReleaseResources);

    return vk::Result::eSuccess;
}

const CommandBuffer::RenderPassBinding& CommandBuffer::get_current_render_pass() const
{
    return current_render_pass;
}

uint32_t CommandBuffer::get_current_subpass_index() const
{
    return pipeline_state.get_subpass_index();
}

bool CommandBuffer::is_render_size_optimal(const vk::Extent2D& extent, const vk::Rect2D& render_area) const
{
    const auto render_area_granularity = current_render_pass.render_pass->get_render_area_granularity();
    return ((render_area.offset.x % render_area_granularity.width == 0) && (render_area.offset.y % render_area_granularity.height == 0) &&
        ((render_area.extent.width % render_area_granularity.width == 0) || (render_area.offset.x + render_area.extent.width == extent.
            width)) &&
        ((render_area.extent.height % render_area_granularity.height == 0) || (render_area.offset.y + render_area.extent.height == extent.
            height)));
}

void CommandBuffer::flush_pipeline_state(const vk::PipelineBindPoint pipeline_bind_point)
{
    // Create a new pipeline only if the graphics state changed
    if (!pipeline_state.is_dirty())
        return;

    pipeline_state.clear_dirty();

    // Create and bind pipeline
    if (pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
    {
        pipeline_state.set_render_pass(*current_render_pass.render_pass);
        auto& pipeline = get_device().get_resource_cache().request_graphics_pipeline(pipeline_state);

        get_handle().bindPipeline(pipeline_bind_point, pipeline.get_handle());
    }
    else if (pipeline_bind_point == vk::PipelineBindPoint::eCompute)
    {
        auto& pipeline = get_device().get_resource_cache().request_compute_pipeline(pipeline_state);

        get_handle().bindPipeline(pipeline_bind_point, pipeline.get_handle());
    }
    else
    {
        throw std::runtime_error("Only graphics and compute pipeline bind points are supported now");
    }
}

void CommandBuffer::flush_descriptor_state(vk::PipelineBindPoint pipeline_bind_point)
{
    PORTAL_CORE_ASSERT(command_pool.get_render_frame(), "The command pool must have an associated render frame");

    const auto& pipeline_layout = pipeline_state.get_pipeline_layout();
    std::unordered_set<uint32_t> update_descriptor_sets;

    // Iterate over the shader sets to check if they have already been bound
    // If they have, add the set so that the command buffer later updates it
    for (auto& id : pipeline_layout.get_shader_sets() | std::views::keys)
    {
        auto descriptor_set_layout = descriptor_set_layout_binding_state.find(id);

        if (descriptor_set_layout != descriptor_set_layout_binding_state.end())
        {
            if (descriptor_set_layout->second->get_handle() != pipeline_layout.get_descriptor_set_layout(id).get_handle())
                update_descriptor_sets.emplace(id);
        }
    }


    // Validate that the bound descriptor set layouts exist in the pipeline layout
    for (auto set_it = descriptor_set_layout_binding_state.begin(); set_it != descriptor_set_layout_binding_state.end();)
    {
        if (!pipeline_layout.has_descriptor_set_layout(set_it->first))
            set_it = descriptor_set_layout_binding_state.erase(set_it);
        else
            ++set_it;
    }

    // Check if a descriptor set needs to be created
    if (resource_binding_state.is_dirty() || !update_descriptor_sets.empty())
    {
        resource_binding_state.clear_dirty();

        // Iterate over all of the resource sets bound by the command buffer
        for (auto& [descriptor_set_id, resource_set] : resource_binding_state.get_resource_sets())
        {
            // Don't update resource set if it's not in the update list OR its state hasn't changed
            if (!resource_set.is_dirty() && (!update_descriptor_sets.contains(descriptor_set_id)))
                continue;

            // Clear dirty flag for resource set
            resource_binding_state.clear_dirty(descriptor_set_id);

            // Skip resource set if a descriptor set layout doesn't exist for it
            if (!pipeline_layout.has_descriptor_set_layout(descriptor_set_id))\
                continue;

            auto& descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(descriptor_set_id);

            // Make descriptor set layout bound for current set
            descriptor_set_layout_binding_state[descriptor_set_id] = &descriptor_set_layout;
            BindingMap<vk::DescriptorBufferInfo> buffer_infos;
            BindingMap<vk::DescriptorImageInfo> image_infos;
            std::vector<uint32_t> dynamic_offsets;

            // Iterate over all resource bindings
            for (auto& [index, binding_resources] : resource_set.get_resource_bindings())
            {
                // Check if binding exists in the pipeline layout
                if (auto binding_info = descriptor_set_layout.get_layout_binding(index))
                {
                    // Iterate over all binding resources
                    for (auto& [array_element, resource_info] : binding_resources)
                    {
                        // Pointer references
                        auto& buffer = resource_info.buffer;
                        auto& sampler = resource_info.sampler;
                        auto& image_view = resource_info.image_view;

                        // Get buffer info
                        if (buffer != nullptr && is_buffer_descriptor_type(binding_info->descriptorType))
                        {
                            vk::DescriptorBufferInfo buffer_info(resource_info.buffer->get_handle(), resource_info.offset, resource_info.range);
                            if (is_dynamic_buffer_descriptor_type(binding_info->descriptorType))
                            {
                                dynamic_offsets.push_back(buffer_info.offset);
                                buffer_info.offset = 0;
                            }

                            buffer_infos[index][array_element] = buffer_info;
                        }
                        // Get image info
                        else if (image_view != nullptr || sampler != nullptr)
                        {
                            // Can be null for input attachments
                            vk::DescriptorImageInfo image_info(sampler ? sampler->get_handle() : nullptr, image_view->get_handle());

                            if (image_view != nullptr)
                            {
                                // Add image layout info based on descriptor type
                                switch (binding_info->descriptorType)
                                {
                                case vk::DescriptorType::eCombinedImageSampler:
                                    image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                                    break;
                                case vk::DescriptorType::eInputAttachment:
                                    if (is_depth_format(image_view->get_format()))
                                        image_info.imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
                                    else
                                        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                                    break;
                                case vk::DescriptorType::eStorageImage:
                                    image_info.imageLayout = vk::ImageLayout::eGeneral;
                                    break;
                                default:
                                    continue;
                                }
                            }

                            image_infos[index][array_element] = image_info;
                        }

                        PORTAL_CORE_ASSERT(
                            (!update_after_bind ||(buffer_infos.contains(index)|| (image_infos.contains(index)))),
                            "binding index with no buffer or image infos can't be checked for adding to bindings_to_update"
                        );
                    }
                }
            }

            vk::DescriptorSet descriptor_set_handle = command_pool.get_render_frame()->request_descriptor_set(
                descriptor_set_layout,
                buffer_infos,
                image_infos,
                update_after_bind,
                command_pool.get_thread_index()
            );

            // Bind descriptor set
            get_handle().bindDescriptorSets(
                pipeline_bind_point,
                pipeline_layout.get_handle(),
                descriptor_set_id,
                descriptor_set_handle,
                dynamic_offsets
            );
        }
    }
}

void CommandBuffer::flush_push_constants()
{
    if (stored_push_constants.empty())
        return;

    auto& pipeline_layout = pipeline_state.get_pipeline_layout();
    const auto shader_stage = pipeline_layout.get_push_constant_range_stage(stored_push_constants.size());
    if (shader_stage)
        get_handle().pushConstants<uint8_t>(pipeline_layout.get_handle(), shader_stage, 0, stored_push_constants);
    else
        LOG_CORE_WARN_TAG("Vulkan", "Push constant range [{}, {}] not found", 0, stored_push_constants.size());
    stored_push_constants.clear();
}
} // portal
