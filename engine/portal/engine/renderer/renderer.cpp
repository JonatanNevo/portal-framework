//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

// #define VMA_STATIC_VULKAN_FUNCTIONS 0
// #define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
// #define VMA_IMPLEMENTATION
// #include "vma/vk_mem_alloc.h"

#include "renderer.h"

#include "portal/core/log.h"

#include <glm/gtc/matrix_transform.hpp>

#include "vulkan/vulkan_utils.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "portal/engine/renderer/descriptor_layout_builder.h"

#include <imgui.h>
#include <ranges>

#include "portal/engine/imgui/backends/imgui_impl_vulkan.h"

#include "llvm/Support/MemoryBuffer.h"
#include "portal/application/settings.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/descriptor_writer.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/renderer/pipeline/pipeline.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "vulkan/device/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/vulkan_pipeline.h"
#include "vulkan/render_target/vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"

#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/input/input_events.h"

namespace portal
{
using namespace renderer;

[[maybe_unused]] static auto logger = Log::get_logger("Renderer");

Renderer::Renderer(vulkan::VulkanContext& context, ResourceRegistry& resource_registry)
    : context(context),
      renderer_context(context)
{
    init_global_descriptors(resource_registry);
    is_initialized = true;
}

Renderer::~Renderer()
{
    cleanup();
}

void Renderer::cleanup()
{
    PORTAL_PROF_ZONE();
    if (is_initialized)
    {
        context.get_device().wait_idle();
        deletion_queue.flush();
    }
    is_initialized = false;
}

void Renderer::begin_frame(const FrameContext&)
{
    PORTAL_PROF_ZONE();
    PORTAL_ASSERT(is_initialized, "Renderer is not initialized");
}

void Renderer::end_frame(FrameContext&)
{
}

const RendererContext& Renderer::get_renderer_context() const
{
    return renderer_context;
}

void Renderer::post_update(FrameContext& frame)
{
    const auto* rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

    // TODO: this should be based on what im rendering rn, add some `renderpass` class?
    const auto draw_image = rendering_context->render_target.lock()->get_image(0);
    const auto depth_draw_image = rendering_context->render_target.lock()->get_depth_image();

    vulkan::transition_image_layout(
        rendering_context->global_command_buffer,
        draw_image,
        1,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eNone,
        vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
    );
    vulkan::transition_image_layout(
        rendering_context->global_command_buffer,
        depth_draw_image,
        1,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests,
        vk::ImageAspectFlagBits::eDepth
    );

    draw_geometry(frame, rendering_context->global_command_buffer);

    // set draw image layout to Present so we can present it
    vulkan::transition_image_layout(
        rendering_context->global_command_buffer,
        draw_image,
        1,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::ImageAspectFlagBits::eColor
    );
}

void Renderer::draw_geometry(FrameContext& frame, const vk::CommandBuffer& command_buffer)
{
    PORTAL_PROF_ZONE();

    auto* rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

    //reset counters
    frame.stats.drawcall_count = 0;
    frame.stats.triangle_count = 0;
    //begin clock
    auto start = std::chrono::system_clock::now();

    std::unordered_map<StringId, std::vector<uint32_t>> render_by_material;
    render_by_material.reserve(rendering_context->render_objects.size());

    for (uint32_t i = 0; i < rendering_context->render_objects.size(); i++)
    {
        const auto& object = rendering_context->render_objects[i];
        if (object.is_visible(rendering_context->scene_data.view_proj))
            render_by_material[object.material->get_id()].push_back(i);
    }

    {
        auto info = reference_cast<vulkan::VulkanRenderTarget>(rendering_context->render_target.lock())->make_rendering_info();
        command_buffer.beginRendering(info);

        scene_data_uniform_buffer->get(frame.frame_index)->set_data_typed<vulkan::GPUSceneData>(rendering_context->scene_data);

        Reference<vulkan::VulkanPipeline> last_pipeline = nullptr;
        Reference<vulkan::VulkanMaterial> last_material = nullptr;
        // MaterialPipeline* real_p = nullptr;
        std::shared_ptr<vulkan::AllocatedBuffer> last_index_buffer = nullptr;

        auto draw_object = [&](const RenderObject& object)
        {
            // TracyVkZone(tracy_context, *current_rendering_context->command_buffer, "Draw object");
            auto material = reference_cast<vulkan::VulkanMaterial>(object.material);
            auto pipeline = material->get_pipeline();
            if (material != last_material)
            {
                last_material = material;
                //rebind pipeline and descriptors if the material changed
                if (pipeline != last_pipeline)
                {
                    last_pipeline = pipeline;

                    descriptor_set_manager->invalidate_and_update(frame.frame_index);

                    const auto descriptor_sets =
                        std::ranges::to<std::vector>(
                            descriptor_set_manager->get_descriptor_sets(frame.frame_index) | std::views::transform(
                                [](const auto& set) { return *set; }
                            )
                        );

                    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->get_vulkan_pipeline());
                    command_buffer.bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics,
                        pipeline->get_vulkan_pipeline_layout(),
                        0,
                        descriptor_sets,
                        {}
                    );

                    command_buffer.setViewport(
                        0,
                        vk::Viewport(
                            0.0f,
                            0.0f,
                            static_cast<float>(rendering_context->viewport_bounds.z),
                            static_cast<float>(rendering_context->viewport_bounds.w),
                            0.0f,
                            1.0f
                        )
                    );
                    command_buffer.setScissor(
                        0,
                        vk::Rect2D(
                            vk::Offset2D(0, 0),
                            {
                                static_cast<uint32_t>(rendering_context->viewport_bounds.z),
                                static_cast<uint32_t>(rendering_context->viewport_bounds.w),
                            }
                        )
                    );
                }

                const auto descriptor_set = material->get_descriptor_set(frame.frame_index);
                const std::vector descriptor_set_array{
                    descriptor_set
                };

                command_buffer.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    pipeline->get_vulkan_pipeline_layout(),
                    1,
                    descriptor_set_array,
                    {}
                );
            }

            //rebind index buffer if needed
            if (object.index_buffer != last_index_buffer)
            {
                last_index_buffer = object.index_buffer;
                command_buffer.bindIndexBuffer(object.index_buffer->get_handle(), 0, vk::IndexType::eUint32);
            }

            vulkan::GPUDrawPushConstants push_constants{
                object.transform,
                object.vertex_buffer_address,
            };
            command_buffer.pushConstants<vulkan::GPUDrawPushConstants>(
                pipeline->get_vulkan_pipeline_layout(),
                vk::ShaderStageFlagBits::eVertex,
                0,
                {push_constants}
            );

            command_buffer.drawIndexed(object.index_count, 1, object.first_index, 0, 0);

            //add counters for triangles and draws
            frame.stats.drawcall_count++;
            frame.stats.triangle_count += static_cast<uint32_t>(object.index_count / 3);
        };

        for (const auto& indexes : render_by_material | std::views::values)
        {
            for (const auto& index : indexes)
            {
                draw_object(rendering_context->render_objects[index]);
            }
        }

        command_buffer.endRendering();
    }

    auto end = std::chrono::system_clock::now();

    //convert to microseconds (integer), and then come back to milliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    frame.stats.mesh_draw_time = elapsed.count() / 1000.f;
}

void Renderer::immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function)
{
    PORTAL_PROF_ZONE();
    context.get_device().get_handle().resetFences({immediate_fence});
    immediate_command_buffer.reset();

    immediate_command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    {
        // TracyVkZone(tracy_context, *immediate_command_buffer, "Immediate Command Buffer");
        function(immediate_command_buffer);
    }
    // TracyVkCollect(tracy_context, *immediate_command_buffer);
    immediate_command_buffer.end();

    vk::CommandBufferSubmitInfo cmd_submit_info{
        .commandBuffer = immediate_command_buffer,
        .deviceMask = 0,
    };

    vk::SubmitInfo2 submit_info{
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_submit_info
    };

    context.get_device().get_graphics_queue().submit({submit_info}, immediate_fence);
    context.get_device().wait_for_fences({immediate_fence}, true, std::numeric_limits<uint64_t>::max());
}

void Renderer::init_global_descriptors(ResourceRegistry& resource_registry)
{
    // TODO: should this be here or under scene?
    auto frames_in_flight = Settings::get().get_setting<size_t>("application.frames_in_flight", 3);

    auto shader = resource_registry.immediate_load<Shader>(STRING_ID("engine/shaders/pbr"));
    const auto hash = shader->compile_with_permutations({});
    const auto variant = shader->get_shader(hash).lock();

    const DescriptorSetManagerProperties manager_props{
        .shader = variant,
        .debug_name = STRING_ID("Global Set Manager"),
        .start_set = 0,
        .end_set = 1,
        .frame_in_flights = frames_in_flight
    };
    descriptor_set_manager = vulkan::VulkanDescriptorSetManager::create_unique(manager_props, context.get_device());

    scene_data_uniform_buffer = make_reference<vulkan::VulkanUniformBufferSet>(
        sizeof(vulkan::GPUSceneData),
        frames_in_flight,
        context.get_device()
    );
    descriptor_set_manager->set_input(STRING_ID("scene_data"), scene_data_uniform_buffer);


    descriptor_set_manager->bake();
}
} // portal
