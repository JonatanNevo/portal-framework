//
// Created by Jonatan Nevo on 15/03/2025.
//

#include "gui.h"

#include <numeric>

#include "portal/application/module/renderer/renderer.h"
#include "portal/application/module/renderer/gui/utils.h"
#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/base/buffer_pool.h"
#include "portal/application/vulkan/rendering/render_frame.h"
#include "portal/application/window/window.h"

namespace portal::gui
{
static void upload_draw_data(const ImDrawData* draw_data, uint8_t* vertex_data, uint8_t* index_data)
{
    auto vertex_dst = reinterpret_cast<ImDrawVert*>(vertex_data);
    auto index_dst = reinterpret_cast<ImDrawIdx*>(index_data);

    for (int i = 0; i < draw_data->CmdListsCount; i++)
    {
        const auto* cmd_list = draw_data->CmdLists[i];
        std::memcpy(vertex_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(index_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vertex_dst += cmd_list->VtxBuffer.Size;
        index_dst += cmd_list->IdxBuffer.Size;
    }
}

void reset_graph_max_value(vulkan::stats::StatGraphData& graph_data)
{
    // If it does not have a fixed max
    if (!graph_data.has_fixed_max)
    {
        // Reset it
        graph_data.max_value = 0.0f;
    }
}

bool Gui::visible = true;
const double Gui::press_time_ms = 200.0f;
const float Gui::overlay_alpha = 0.3f;
const std::string Gui::default_font = "Roboto-Regular";
const ImGuiWindowFlags Gui::common_flags = ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoFocusOnAppearing;
const ImGuiWindowFlags Gui::options_flags = Gui::common_flags;
const ImGuiWindowFlags Gui::info_flags = Gui::common_flags | ImGuiWindowFlags_NoInputs;

Gui::StatsView::StatsView(const vulkan::Stats* stats)
{
    if (stats == nullptr)
    {
        return;
    }

    // Request graph data information for each stat and record it in graph_map
    const std::set<vulkan::stats::StatIndex>& indices = stats->get_requested_stats();

    for (vulkan::stats::StatIndex i : indices)
    {
        graph_map[i] = stats->get_graph_data(i);
    }
}

void Gui::StatsView::reset_max_values()
{
    // For every entry in the map
    std::ranges::for_each(graph_map, [](auto& pr) { reset_graph_max_value(pr.second); });
}

void Gui::StatsView::reset_max_value(vulkan::stats::StatIndex index)
{
    const auto pr = graph_map.find(index);
    if (pr != graph_map.end())
    {
        reset_graph_max_value(pr->second);
    }
}

Gui::Gui(Renderer& renderer, const Window& window, const vulkan::Stats* stats, float font_size, bool explicit_update)
    : renderer(renderer),
      content_scale_factor(window.get_content_scale_factor()),
      dpi_factor(window.get_dpi_factor() * content_scale_factor),
      explicit_update(explicit_update),
      stats_view(stats)
{
    ImGui::CreateContext();

    ImGuiStyle& style = ImGui::GetStyle();

    // Color scheme
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.005f, 0.005f, 0.005f, 0.94f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);

    // Borderless window
    style.WindowBorderSize = 0.0f;

    // Global scale
    style.ScaleAllSizes(dpi_factor);

    // Dimensions
    ImGuiIO& io = ImGui::GetIO();
    auto const& extent = renderer.get_render_context().get_surface_extent();
    io.DisplaySize.x = static_cast<float>(extent.width);
    io.DisplaySize.y = static_cast<float>(extent.height);
    io.FontGlobalScale = 1.0f;
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Enable keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Default font
    fonts.emplace_back(default_font, font_size * dpi_factor);

    // Debug window font
    fonts.emplace_back("RobotoMono-Regular", (font_size / 2) * dpi_factor);

    // Create font texture
    unsigned char* font_data;
    int tex_width, tex_height;
    io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
    const size_t upload_size = tex_width * tex_height * 4 * sizeof(char);

    auto& device = renderer.get_render_context().get_device();
    font_image = vulkan::ImageBuilder(tex_width, tex_height)
                 .with_format(vk::Format::eR8G8B8A8Unorm)
                 .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
                 .with_debug_name("GUI font image")
                 .build_unique(device);
    font_image_view = std::make_unique<vulkan::ImageView>(*font_image, vk::ImageViewType::e2D);
    font_image_view->set_debug_name("View on GUI font image");

    // Upload font data into the vulkan image memory
    {
        const auto stage_buffer = vulkan::Buffer::create_staging_buffer(device, upload_size, font_data);
        auto& command_buffer = device.get_command_pool().request_command_buffer();

        vulkan::FencePool fence_pool(device);

        // Begin recording
        command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        {
            // Prepare for transfer
            constexpr vulkan::ImageMemoryBarrier memory_barrier{
                .old_layout = vk::ImageLayout::eUndefined,
                .new_layout = vk::ImageLayout::eTransferDstOptimal,
                .src_access_mask = {},
                .dst_access_mask = vk::AccessFlagBits::eTransferWrite,
                .src_stage_mask = vk::PipelineStageFlagBits::eHost,
                .dst_stage_mask = vk::PipelineStageFlagBits::eTransfer,
            };

            command_buffer.image_memory_barrier(*font_image_view, memory_barrier);
        }

        // Copy
        vk::BufferImageCopy buffer_copy_region{};
        buffer_copy_region.imageSubresource.layerCount = font_image_view->get_subresource_range().layerCount;
        buffer_copy_region.imageSubresource.aspectMask = font_image_view->get_subresource_range().aspectMask;
        buffer_copy_region.imageExtent = font_image->get_extent();

        command_buffer.copy_buffer_to_image(stage_buffer, *font_image, {buffer_copy_region});

        {
            // Prepare for fragment shader
            constexpr vulkan::ImageMemoryBarrier memory_barrier{
                .old_layout = vk::ImageLayout::eTransferDstOptimal,
                .new_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                .src_access_mask = vk::AccessFlagBits::eTransferWrite,
                .dst_access_mask = vk::AccessFlagBits::eShaderRead,
                .src_stage_mask = vk::PipelineStageFlagBits::eTransfer,
                .dst_stage_mask = vk::PipelineStageFlagBits::eFragmentShader,
            };

            command_buffer.image_memory_barrier(*font_image_view, memory_barrier);
        }

        // End recording
        command_buffer.end();

        auto& queue = device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
        queue.submit(command_buffer, device.get_fence_pool().request_fence());

        // Wait for the command buffer to finish its work before destroying the staging buffer
        device.get_fence_pool().wait();
        device.get_fence_pool().reset();
        device.get_command_pool().reset_pool();
    }

    vulkan::ShaderSource vert_shader("imgui.vert");
    vulkan::ShaderSource frag_shader("imgui.frag");

    std::vector<vulkan::ShaderModule*> shader_modules;
    shader_modules.push_back(&device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, vert_shader, {}));
    shader_modules.push_back(&device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, frag_shader, {}));

    pipeline_layout = &device.get_resource_cache().request_pipeline_layout(shader_modules);

    // Determine the filtering to be used, based on what is supported for the format
    const vk::FormatProperties fmt_props = device.get_gpu().get_handle().getFormatProperties(font_image_view->get_format());

    vk::Filter filter = (fmt_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)
                            ? vk::Filter::eLinear
                            : vk::Filter::eNearest;

    // Create texture sampler
    vk::SamplerCreateInfo sampler_info;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.magFilter = filter;
    sampler_info.minFilter = filter;
    sampler_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
    sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    sampler_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;

    sampler = std::make_unique<vulkan::Sampler>(device, sampler_info);
    sampler->set_debug_name("GUI sampler");

    if (explicit_update)
    {
        vertex_buffer = vulkan::BufferBuilder(1)
                        .with_usage(vk::BufferUsageFlagBits::eVertexBuffer)
                        .with_vma_usage(vma::MemoryUsage::eGpuToCpu)
                        .with_debug_name("GUI vertex buffer")
                        .build_shared(device);

        index_buffer = vulkan::BufferBuilder(1)
                       .with_usage(vk::BufferUsageFlagBits::eIndexBuffer)
                       .with_vma_usage(vma::MemoryUsage::eGpuToCpu)
                       .with_debug_name("GUI index buffer")
                       .build_shared(device);
    }
}

Gui::~Gui()
{
    const vk::Device device = renderer.get_render_context().get_device().get_handle();
    // descriptor_set is implicitly freed by destroying descriptor_pool!
    device.destroyDescriptorPool(descriptor_pool);
    device.destroyDescriptorSetLayout(descriptor_set_layout);
    device.destroyPipeline(pipeline);

    ImGui::DestroyContext();
}

void Gui::prepare(vk::PipelineCache pipeline_cache, vk::RenderPass render_pass, const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages)
{
    vk::Device device = renderer.get_render_context().get_device().get_handle();

    // Descriptor pool
    vk::DescriptorPoolSize pool_size(vk::DescriptorType::eCombinedImageSampler, 1);
    const vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2, pool_size);
    descriptor_pool = device.createDescriptorPool(descriptor_pool_create_info);

    // Descriptor set layout
    vk::DescriptorSetLayoutBinding layout_binding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    const vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, layout_binding);
    descriptor_set_layout = device.createDescriptorSetLayout(descriptor_set_layout_create_info);


    // Descriptor set
    const vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptor_pool, descriptor_set_layout);
    descriptor_set = device.allocateDescriptorSets(descriptor_set_allocate_info).front();

    vk::DescriptorImageInfo font_descriptor(sampler->get_handle(), font_image_view->get_handle(), vk::ImageLayout::eShaderReadOnlyOptimal);
    const vk::WriteDescriptorSet write_descriptor_set(descriptor_set, 0, 0, vk::DescriptorType::eCombinedImageSampler, font_descriptor);
    device.updateDescriptorSets(write_descriptor_set, {});

    // Setup graphics pipeline for UI rendering
    // Vertex bindings an attributes based on ImGui vertex definition
    vk::VertexInputBindingDescription vertex_input_binding(0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex);
    std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {
        {
            // Location 0: Position
            {0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)},
            // Location 1 : UV
            {1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)},
            // Location 2: Color
            {2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)}
        }
    };
    vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::eTriangleList, false);
    vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);
    vk::PipelineRasterizationStateCreateInfo rasterization_state(
        {},
        false,
        {},
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        {},
        {},
        {},
        {},
        1.0f
    );

    vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);
    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state(
        {},
        false,
        false,
        vk::CompareOp::eAlways,
        {},
        {},
        {},
        {{}, {}, {}, vk::CompareOp::eAlways}
    );
    // Enable blending
    vk::PipelineColorBlendAttachmentState blend_attachment_state(
        true,
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );
    vk::PipelineColorBlendStateCreateInfo color_blend_state({}, {}, {}, blend_attachment_state);

    std::array<vk::DynamicState, 2> dynamic_state_enables = {{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};
    vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

    vk::GraphicsPipelineCreateInfo pipeline_create_info(
        {},
        shader_stages,
        &vertex_input_state,
        &input_assembly_state,
        nullptr,
        &viewport_state,
        &rasterization_state,
        &multisample_state,
        &depth_stencil_state,
        &color_blend_state,
        &dynamic_state,
        pipeline_layout->get_handle(),
        render_pass,
        0,
        nullptr,
        -1
    );

    pipeline = device.createGraphicsPipeline(pipeline_cache, pipeline_create_info).value;
}

void Gui::resize(const uint32_t width, const uint32_t height) const
{
    auto& io = ImGui::GetIO();
    io.DisplaySize.x = static_cast<float>(width);
    io.DisplaySize.y = static_cast<float>(height);
}

void Gui::new_frame() const
{
    ImGui::NewFrame();
}

void Gui::update(const float delta_time)
{
    if (visible != prev_visible)
    {
        drawer.set_dirty(true);
        prev_visible = visible;
    }

    if (!visible)
    {
        ImGui::EndFrame();
        return;
    }

    // Update imGui
    ImGuiIO& io = ImGui::GetIO();
    const auto extent = renderer.get_render_context().get_surface_extent();
    resize(extent.width, extent.height);
    io.DeltaTime = delta_time;

    // Render to generate draw buffers
    ImGui::Render();
}

bool Gui::update_buffers()
{
    ImDrawData* draw_data = ImGui::GetDrawData();
    bool updated = false;

    if (!draw_data)
        return false;

    const size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    const size_t index_buffer_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertex_buffer_size == 0) || (index_buffer_size == 0))
        return false;

    if ((!vertex_buffer->get_handle()) || (vertex_buffer_size != last_vertex_buffer_size))
    {
        last_vertex_buffer_size = vertex_buffer_size;
        updated = true;

        vertex_buffer = vulkan::BufferBuilder(vertex_buffer_size)
                        .with_usage(vk::BufferUsageFlagBits::eVertexBuffer)
                        .with_vma_usage(vma::MemoryUsage::eGpuToCpu)
                        .build_shared(renderer.get_render_context().get_device());
        vertex_buffer->set_debug_name("GUI vertex buffer");
    }

    if ((!index_buffer->get_handle()) || (index_buffer_size != last_index_buffer_size))
    {
        last_index_buffer_size = index_buffer_size;
        updated = true;

        index_buffer = vulkan::BufferBuilder(vertex_buffer_size)
                       .with_usage(vk::BufferUsageFlagBits::eIndexBuffer)
                       .with_vma_usage(vma::MemoryUsage::eGpuToCpu)
                       .build_shared(renderer.get_render_context().get_device());
        index_buffer->set_debug_name("GUI index buffer");
    }

    // Upload data
    upload_draw_data(draw_data, vertex_buffer->map(), index_buffer->map());

    vertex_buffer->flush();
    index_buffer->flush();

    vertex_buffer->unmap();
    index_buffer->unmap();

    return updated;
}

void Gui::draw(vulkan::CommandBuffer& command_buffer)
{
    if (!visible)
        return;

    vulkan::ScopedDebugLabel debug_label(command_buffer, "GUI");

    // Vertex input state
    vk::VertexInputBindingDescription vertex_input_binding({}, sizeof(ImDrawVert));
    // Location 0: Position
    vk::VertexInputAttributeDescription pos_attr(0, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(ImDrawVert, pos)));
    // Location 1: UV
    vk::VertexInputAttributeDescription uv_attr(1, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(ImDrawVert, uv)));
    // Location 2: Color
    vk::VertexInputAttributeDescription col_attr(2, 0, vk::Format::eR8G8B8A8Unorm, static_cast<uint32_t>(offsetof(ImDrawVert, col)));


    vulkan::VertexInputState vertex_input_state;
    vertex_input_state.bindings = {vertex_input_binding};
    vertex_input_state.attributes = {pos_attr, uv_attr, col_attr};

    // Blend state
    vulkan::ColorBlendAttachmentState color_attachment;
    color_attachment.blend_enable = true;
    color_attachment.color_write_mask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
    color_attachment.src_color_blend_factor = vk::BlendFactor::eSrcAlpha;
    color_attachment.dst_color_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;
    color_attachment.src_alpha_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;

    vulkan::ColorBlendState blend_state{};
    blend_state.attachments = {color_attachment};

    vulkan::RasterizationState rasterization_state{};
    rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;

    vulkan::DepthStencilState depth_state{};
    depth_state.depth_test_enable = false;
    depth_state.depth_write_enable = false;

    command_buffer.set_vertex_input_state(vertex_input_state);
    command_buffer.set_color_blend_state(blend_state);
    command_buffer.set_rasterization_state(rasterization_state);
    command_buffer.set_depth_stencil_state(depth_state);

    // Bind pipeline layout
    command_buffer.bind_pipeline_layout(*pipeline_layout);
    command_buffer.bind_image(*font_image_view, *sampler, 0, 0, 0);

    // Pre-rotation
    auto& io = ImGui::GetIO();
    auto push_transform = glm::mat4(1.0f);

    if (renderer.get_render_context().has_swapchain())
    {
        auto transform = renderer.get_render_context().get_swapchain().get_transform();

        auto rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f);
        if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate90)
        {
            push_transform = glm::rotate(push_transform, glm::radians(90.0f), rotation_axis);
        }
        else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate270)
        {
            push_transform = glm::rotate(push_transform, glm::radians(270.0f), rotation_axis);
        }
        else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate180)
        {
            push_transform = glm::rotate(push_transform, glm::radians(180.0f), rotation_axis);
        }
    }

    // GUI coordinate space to screen space
    push_transform = glm::translate(push_transform, glm::vec3(-1.0f, -1.0f, 0.0f));
    push_transform = glm::scale(push_transform, glm::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));

    // Push constants
    command_buffer.push_constants(push_transform);

    std::vector<std::reference_wrapper<const vulkan::Buffer>> vertex_buffers;
    std::vector<vk::DeviceSize> vertex_offsets;

    // If a render context is used, then use the frames buffer pools to allocate GUI vertex/index data from
    if (!explicit_update)
    {
        // Save vertex buffer allocation in case we need to rebind with vertex_offset, e.g. for iOS Simulator
        auto vertex_allocation = update_buffers(command_buffer);
        if (!vertex_allocation.empty())
        {
            vertex_buffers.emplace_back(vertex_allocation.get_buffer());
            vertex_offsets.push_back(vertex_allocation.get_offset());
        }
    }
    else
    {
        vertex_buffers.emplace_back(*vertex_buffer);
        vertex_offsets.push_back(0);
        command_buffer.bind_vertex_buffers(0, vertex_buffers, vertex_offsets);

        command_buffer.bind_index_buffer(*index_buffer, 0, vk::IndexType::eUint16);
    }

    // Render commands
    ImDrawData* draw_data = ImGui::GetDrawData();
    int32_t vertex_offset = 0;
    uint32_t index_offset = 0;

    if (!draw_data || draw_data->CmdListsCount == 0)
    {
        return;
    }

    for (int32_t i = 0; i < draw_data->CmdListsCount; i++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];
        for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
        {
            const ImDrawCmd* cmd = &cmd_list->CmdBuffer[j];
            vk::Rect2D scissor_rect;
            scissor_rect.offset.x = std::max(static_cast<int32_t>(cmd->ClipRect.x), 0);
            scissor_rect.offset.y = std::max(static_cast<int32_t>(cmd->ClipRect.y), 0);
            scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
            scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);

            // Adjust for pre-rotation if necessary
            if (renderer.get_render_context().has_swapchain())
            {
                auto transform = renderer.get_render_context().get_swapchain().get_transform();
                if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate90)
                {
                    scissor_rect.offset.x = static_cast<int32_t>(io.DisplaySize.y - cmd->ClipRect.w);
                    scissor_rect.offset.y = static_cast<int32_t>(cmd->ClipRect.x);
                    scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
                    scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
                }
                else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate180)
                {
                    scissor_rect.offset.x = static_cast<int32_t>(io.DisplaySize.x - cmd->ClipRect.z);
                    scissor_rect.offset.y = static_cast<int32_t>(io.DisplaySize.y - cmd->ClipRect.w);
                    scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
                    scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
                }
                else if (transform & vk::SurfaceTransformFlagBitsKHR::eRotate270)
                {
                    scissor_rect.offset.x = static_cast<int32_t>(cmd->ClipRect.y);
                    scissor_rect.offset.y = static_cast<int32_t>(io.DisplaySize.x - cmd->ClipRect.z);
                    scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
                    scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
                }
            }

            command_buffer.set_scissor(0, {scissor_rect});
            command_buffer.draw_indexed(cmd->ElemCount, 1, index_offset, vertex_offset, 0);
            index_offset += cmd->ElemCount;
        }
        vertex_offset += cmd_list->VtxBuffer.Size;
    }
}

void Gui::show_top_window(const std::string& app_name, const vulkan::Stats* stats, const debug::DebugInfo* debug_info)
{
    // Transparent background
    ImGui::SetNextWindowBgAlpha(overlay_alpha);
    const ImVec2 size{ImGui::GetIO().DisplaySize.x, 0.0f};
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    // Top left
    constexpr ImVec2 pos{0.0f, 0.0f};
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

    bool is_open = true;
    ImGui::Begin("Top", &is_open, common_flags);

    show_app_info(app_name);

    if (stats)
    {
        show_stats(*stats);

        // Reset max values if user taps on this window
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0 /* left */))
        {
            stats_view.reset_max_values();
        }
    }

    if (debug_info)
    {
        if (debug_view.active)
        {
            show_debug_window(*debug_info, ImVec2{0, ImGui::GetWindowSize().y});
        }
    }

    ImGui::End();
}

void Gui::show_demo_window() const
{
    ImGui::ShowDemoWindow();
}

void Gui::show_app_info(const std::string& app_name) const
{
    // Sample name
    ImGui::Text("%s", app_name.c_str());
    // GPU name
    const auto& device = renderer.get_render_context().get_device();
    const auto device_name_label = "GPU: " + std::string(device.get_gpu().get_properties().deviceName.data());
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(device_name_label.c_str()).x);
    ImGui::Text("%s", device_name_label.c_str());
}

void Gui::show_debug_window(const debug::DebugInfo& debug_info, const ImVec2& position)
{
    auto& io = ImGui::GetIO();
    auto& style = ImGui::GetStyle();
    auto& font = get_font("RobotoMono-Regular");

    // Calculate only once
    if (debug_view.label_column_width == 0)
    {
        debug_view.label_column_width = style.ItemInnerSpacing.x + debug_info.get_longest_label() * font.size / debug_view.scale;
    }

    ImGui::SetNextWindowBgAlpha(overlay_alpha);
    ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowContentSize(ImVec2{io.DisplaySize.x, 0.0f});

    bool is_open = true;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;

    ImGui::Begin("Debug Window", &is_open, flags);
    ImGui::PushFont(font.handle);

    auto field_count = debug_info.get_fields().size() > debug_view.max_fields ? debug_view.max_fields : debug_info.get_fields().size();

    ImGui::BeginChild("Table", ImVec2(0, static_cast<float>(field_count) * (font.size + style.ItemSpacing.y)), false);
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, debug_view.label_column_width);
    ImGui::SetColumnWidth(1, io.DisplaySize.x - debug_view.label_column_width);
    for (auto& field : debug_info.get_fields())
    {
        const std::string& label = field->label;
        const std::string& value = field->to_string();
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();
        ImGui::Text(" %s", value.c_str());
        ImGui::NextColumn();
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::PopFont();
    ImGui::End();
}

void Gui::show_stats(const vulkan::Stats& stats)
{
    for (const auto& stat_index : stats.get_requested_stats())
    {
        // Find the graph data of this stat index
        auto pr = stats_view.graph_map.find(stat_index);

        // Draw graph
        auto& graph_data = pr->second;
        const auto& graph_elements = stats.get_data(stat_index);
        float graph_min = 0.0f;
        float& graph_max = graph_data.max_value;

        if (!graph_data.has_fixed_max)
        {
            const auto new_max = *std::ranges::max_element(graph_elements) * stats_view.top_padding;
            if (new_max > graph_max)
            {
                graph_max = new_max;
            }
        }

        const auto graph_size = ImVec2{
            ImGui::GetIO().DisplaySize.x,
            stats_view.graph_height /* dpi */ * dpi_factor
        };

        std::stringstream graph_label;
        const float avg = std::accumulate(graph_elements.begin(), graph_elements.end(), 0.0f) / static_cast<float>(graph_elements.size());

        // Check if the stat is available in the current platform
        if (stats.is_available(stat_index))
        {
            graph_label << fmt::format(graph_data.name + ": " + graph_data.format, avg * graph_data.scale_factor);
            ImGui::BeginDisabled();
            ImGui::PlotLines(
                "",
                &graph_elements[0],
                static_cast<int>(graph_elements.size()),
                0,
                graph_label.str().c_str(),
                graph_min,
                graph_max,
                graph_size
            );
            ImGui::EndDisabled();
        }
        else
        {
            graph_label << graph_data.name << ": not available";
            ImGui::Text("%s", graph_label.str().c_str());
        }
    }
}

void Gui::show_options_window(const std::function<void()>& body, const uint32_t lines) const
{
    // Add padding around the text so that the options are not
    // too close to the edges and are easier to interact with.
    // Also add double vertical padding to avoid rounded corners.

    const float window_padding = ImGui::CalcTextSize("T").x;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{window_padding, window_padding * 2.0f});

    const auto window_height = static_cast<float>(lines) * ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2.0f;
    const auto window_width = ImGui::GetIO().DisplaySize.x;
    ImGui::SetNextWindowBgAlpha(overlay_alpha);

    const auto size = ImVec2(window_width, 0);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    const auto pos = ImVec2(0.0f, ImGui::GetIO().DisplaySize.y - window_height);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

    const ImGuiWindowFlags flags = (ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_AlwaysUseWindowPadding |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing);

    bool is_open = true;
    ImGui::Begin("Options", &is_open, flags);
    body();
    ImGui::End();
    ImGui::PopStyleVar();
}

void Gui::show_simple_window(const std::string& name, const uint32_t last_fps, const std::function<void()>& body) const
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::NewFrame();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Portal Engine", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted(name.c_str());
    ImGui::TextUnformatted(renderer.get_render_context().get_device().get_gpu().get_properties().deviceName.data());
    ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / static_cast<float>(last_fps)), last_fps);
    ImGui::PushItemWidth(110.0f * dpi_factor);

    body();

    ImGui::PopItemWidth();
    ImGui::End();
    ImGui::PopStyleVar();
}

bool Gui::input_event(const InputEvent& input_event)
{
    auto& io = ImGui::GetIO();
    auto capture_move_event = false;

    if (input_event.get_source() == EventSource::Keyboard)
    {
        const auto& key_event = static_cast<const KeyInputEvent&>(input_event);

        if (key_event.get_action() == KeyAction::Down)
        {
            io.AddKeyEvent(to_imgui_key(key_event.get_code()), true);
        }
        else if (key_event.get_action() == KeyAction::Up)
        {
            io.AddKeyEvent(to_imgui_key(key_event.get_code()), false);
        }
    }
    else if (input_event.get_source() == EventSource::Mouse)
    {
        const auto& mouse_button = static_cast<const MouseButtonInputEvent&>(input_event);

        io.MousePos = ImVec2{
            mouse_button.get_pos_x() * content_scale_factor,
            mouse_button.get_pos_y() * content_scale_factor
        };

        const auto button_id = static_cast<int>(mouse_button.get_button());

        if (mouse_button.get_action() == MouseAction::Down)
        {
            io.AddMouseButtonEvent(button_id, true);
        }
        else if (mouse_button.get_action() == MouseAction::Up)
        {
            io.AddMouseButtonEvent(button_id, false);
        }
        else if (mouse_button.get_action() == MouseAction::Move)
        {
            capture_move_event = io.WantCaptureMouse;
        }
    }
    else if (input_event.get_source() == EventSource::Touchscreen)
    {
        const auto& touch_event = static_cast<const TouchInputEvent&>(input_event);

        io.MousePos = ImVec2{touch_event.get_pos_x(), touch_event.get_pos_y()};

        if (touch_event.get_action() == TouchAction::Down)
        {
            io.AddMouseButtonEvent(touch_event.get_pointer_id(), true);
        }
        else if (touch_event.get_action() == TouchAction::Up)
        {
            io.AddMouseButtonEvent(touch_event.get_pointer_id(), false);
        }
        else if (touch_event.get_action() == TouchAction::Move)
        {
            capture_move_event = io.WantCaptureMouse;
        }
    }

    // Toggle GUI elements when tap or clicking outside the GUI windows
    if (!io.WantCaptureMouse)
    {
        bool press_down = (
                input_event.get_source() == EventSource::Mouse && static_cast<const MouseButtonInputEvent&>(input_event).get_action() ==
                MouseAction::Down)
            || (input_event.get_source() == EventSource::Touchscreen && static_cast<const TouchInputEvent&>(input_event).get_action() ==
                TouchAction::Down);
        bool press_up = (
                input_event.get_source() == EventSource::Mouse && static_cast<const MouseButtonInputEvent&>(input_event).get_action() ==
                MouseAction::Up)
            || (input_event.get_source() == EventSource::Touchscreen && static_cast<const TouchInputEvent&>(input_event).get_action() ==
                TouchAction::Up);

        // TODO: Move this logic to a callback or something idk
        if (press_down)
        {
            timer.start();
            if (input_event.get_source() == EventSource::Touchscreen)
            {
                const auto& touch_event = static_cast<const TouchInputEvent&>(input_event);
                if (touch_event.get_touch_points() == 2)
                {
                    two_finger_tap = true;
                }
            }
        }
        if (press_up)
        {
            const auto press_delta = timer.stop<Timer::Milliseconds>();
            if (press_delta < press_time_ms)
            {
                if (input_event.get_source() == EventSource::Mouse)
                {
                    const auto& mouse_button = static_cast<const MouseButtonInputEvent&>(input_event);
                    if (mouse_button.get_button() == MouseButton::Right)
                    {
                        debug_view.active = !debug_view.active;
                    }
                }
                else if (input_event.get_source() == EventSource::Touchscreen)
                {
                    const auto& touch_event = static_cast<const TouchInputEvent&>(input_event);
                    if (two_finger_tap && touch_event.get_touch_points() == 2)
                    {
                        debug_view.active = !debug_view.active;
                    }
                    else
                    {
                        two_finger_tap = false;
                    }
                }
            }
        }
    }

    return capture_move_event;
}

const Gui::StatsView& Gui::get_stats_view() const
{
    return stats_view;
}

Drawer& Gui::get_drawer()
{
    return drawer;
}

Font const& Gui::get_font(const std::string& font_name) const
{
    const auto it = std::ranges::find_if(fonts, [&font_name](const Font& font) { return font.name == font_name; });

    if (it != fonts.end())
    {
        return *it;
    }

    LOG_CORE_WARN_TAG("Gui", "Couldn't find font with name {}", font_name);
    return *fonts.begin();
}

bool Gui::is_debug_view_active() const
{
    return debug_view.active;
}

vulkan::BufferAllocation Gui::update_buffers(vulkan::CommandBuffer& command_buffer) const
{
    const ImDrawData* draw_data = ImGui::GetDrawData();
    auto& render_frame = renderer.get_render_context().get_active_frame();

    if (!draw_data || (draw_data->TotalVtxCount == 0) || (draw_data->TotalIdxCount == 0))
    {
        return vulkan::BufferAllocation{};
    }

    const size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    const size_t index_buffer_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

    std::vector<uint8_t> vertex_data(vertex_buffer_size);
    std::vector<uint8_t> index_data(index_buffer_size);

    upload_draw_data(draw_data, vertex_data.data(), index_data.data());

    auto vertex_allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eVertexBuffer, vertex_buffer_size);
    vertex_allocation.update(vertex_data);

    std::vector<std::reference_wrapper<const vulkan::Buffer>> buffers;
    buffers.emplace_back(std::ref(vertex_allocation.get_buffer()));

    command_buffer.bind_vertex_buffers(0, buffers, {vertex_allocation.get_offset()});

    auto index_allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eIndexBuffer, index_buffer_size);
    index_allocation.update(index_data);
    command_buffer.bind_index_buffer(index_allocation.get_buffer(), index_allocation.get_offset(), vk::IndexType::eUint16);

    return vertex_allocation;
}
} // portal
