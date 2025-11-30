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

#include "vulkan/vulkan_init.h"
#include "vulkan/vulkan_utils.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/renderer/pipelines.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <ranges>

#include "llvm/Support/MemoryBuffer.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/descriptor_writer.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/renderer/pipeline/pipeline.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "vulkan/device/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/vulkan_pipeline.h"
#include "portal/engine/renderer/vulkan/vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"

#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/input/input_events.h"

namespace portal
{
using namespace renderer;

[[maybe_unused]] static auto logger = Log::get_logger("Renderer");

Renderer::Renderer(ModuleStack& stack, vulkan::VulkanContext& context, const Reference<vulkan::VulkanSwapchain>& swapchain)
    : TaggedModule(stack, STRING_ID("Renderer")),

      swapchain(swapchain),
      attachments(
          {
              .attachment_images = {
                  // Present Image
                  {
                      .format = vulkan::to_format(swapchain->get_color_format()),
                      .blend = false
                  },
                  // Depth Image
                  {
                      .format = ImageFormat::Depth_32Float,
                      .blend = true,
                      .blend_mode = BlendMode::Additive
                  }
              },
              .blend = true,
          }
      ),
      context(context),
      renderer_context(context, scene_descriptor_set_layouts, attachments)
{
    PORTAL_PROF_ZONE();

    init_render_target();
    init_frame_resources();

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
        frames.clear();

        deletion_queue.flush();
        depth_image.reset();
        scene_descriptor_set_layouts.clear();
        swapchain.reset();
    }
    is_initialized = false;
}

void Renderer::begin_frame(FrameContext& frame)
{
    PORTAL_PROF_ZONE();

    // TODO: pull the `RenderingContext` from some cache instead of allocating each frame
    frame.rendering_context = FrameRenderingContext{
        .depth_image = depth_image->get_image_info().image.get_handle(),
        .depth_image_view = depth_image->get_image_info().view,
        .command_buffer = frames[current_frame].command_buffer,
        .resources = frames[current_frame],
    };

    auto rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

    const auto& [image, image_view, last_used_frame] = swapchain->begin_frame(frame);

    rendering_context->draw_image = image;
    rendering_context->draw_image_view = image_view;


    // If this image was used before, wait for the fence from the frame that last used it
    if (last_used_frame != std::numeric_limits<size_t>::max())
    {
        PORTAL_PROF_ZONE("VulkanSwapchain::begin_frame - wait for image fence");
        const auto& last_frame = frames[last_used_frame];
        context.get_device().wait_for_fences(std::span{&*last_frame.wait_fence, 1}, true);
    }

    // Resets descriptor pools
    clean_frame(frame);

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    rendering_context->command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

void Renderer::end_frame(FrameContext& frame)
{
    const auto* rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

    rendering_context->command_buffer.end();
    // TODO: should we submit to graphics queue here?

    swapchain->present(frame);

    current_frame = (current_frame + 1) % frames_in_flight;
}

const RendererContext& Renderer::get_renderer_context() const
{
    return renderer_context;
}

const renderer::vulkan::VulkanSwapchain& Renderer::get_swapchain() const
{
    return *swapchain;
}

void Renderer::init_render_target()
{
    RenderTargetProperties properties{
        .width = swapchain->get_width(),
        .height = swapchain->get_height(),
        .attachments = attachments,
        .transfer = true,
        .name = STRING_ID("geometry-render-target"),
    };
    render_target = make_reference<vulkan::VulkanRenderTarget>(properties);

    image::Properties image_properties{
        .format = render_target->get_depth_format(),
        .usage = ImageUsage::Attachment,
        .transfer = true,
        .width = static_cast<size_t>(properties.width * properties.scale),
        .height = static_cast<size_t>(properties.height * properties.scale)
    };
    depth_image = make_reference<vulkan::VulkanImage>(image_properties, context);
}

void Renderer::init_frame_resources()
{
    PORTAL_PROF_ZONE();

    // TODO: have different amount of frames in flight based on some config?
    frames_in_flight = swapchain->get_image_count();

    frames.clear();
    frames.reserve(frames_in_flight);

    auto& device = context.get_device();

    // create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<vulkan::DescriptorAllocator::PoolSizeRatio> sizes =
    {
        {vk::DescriptorType::eStorageImage, 1}
    };

    // create synchronization structures
    // one fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to synchronize rendering with swapchain
    // we want the fence to start signaled so we can wait on it on the first frame
    for (size_t i = 0; i < frames_in_flight; ++i)
    {
        const vk::CommandPoolCreateInfo pool_info{
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = static_cast<uint32_t>(context.get_device().get_graphics_queue().get_family_index()),
        };
        auto command_pool = device.get_handle().createCommandPool(pool_info);

        const vk::CommandBufferAllocateInfo alloc_info{
            .commandPool = command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };

        std::vector<vulkan::DescriptorAllocator::PoolSizeRatio> frame_sizes = {
            {vk::DescriptorType::eStorageImage, 3},
            {vk::DescriptorType::eStorageBuffer, 3},
            {vk::DescriptorType::eUniformBuffer, 3},
            {vk::DescriptorType::eCombinedImageSampler, 4},
        };;


        auto& data = frames.emplace_back(
            std::move(command_pool),
            std::move(device.get_handle().allocateCommandBuffers(alloc_info).front()),
            device.get_handle().createSemaphore({}),
            device.get_handle().createSemaphore({}),
            device.get_handle().createFence(
                {
                    .flags = vk::FenceCreateFlagBits::eSignaled
                }
            ),
            vulkan::DescriptorAllocator{context.get_device().get_handle(), 1000, frame_sizes}
        );

        device.set_debug_name(data.command_pool, std::format("swapchain_command_pool_{}", i).c_str());
        device.set_debug_name(data.command_buffer, std::format("swapchain_command_buffer_{}", i).c_str());
        device.set_debug_name(data.image_available_semaphore, std::format("swapchain_image_available_semaphore_{}", i).c_str());
        device.set_debug_name(data.render_finished_semaphore, std::format("swapchain_render_finished_semaphore_{}", i).c_str());
        device.set_debug_name(data.wait_fence, std::format("swapchain_wait_fence_{}", i).c_str());
    }

    {
        vulkan::DescriptorLayoutBuilder builder;
        builder.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
        scene_descriptor_set_layouts.push_back(builder.build(context.get_device().get_handle()));
    }
}

void Renderer::post_update(FrameContext& frame)
{
    const auto* rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

    const auto draw_image = rendering_context->draw_image;
    const auto depth_draw_image = rendering_context->depth_image;

    vulkan::transition_image_layout(
        rendering_context->command_buffer,
        draw_image,
        1,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eMemoryWrite,
        vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
    );
    vulkan::transition_image_layout(
        rendering_context->command_buffer,
        depth_draw_image,
        1,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eMemoryWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests,
        vk::ImageAspectFlagBits::eDepth
    );

    draw_geometry(frame, rendering_context->command_buffer);

    // set draw image layout to Present so we can present it
    vulkan::transition_image_layout(
        rendering_context->command_buffer,
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

void Renderer::clean_frame(const FrameContext& frame)
{
    const auto* rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

    rendering_context->resources.deletion_queue.flush();
    rendering_context->resources.global_descriptor_set = nullptr;
    rendering_context->resources.frame_descriptors.clear_pools();
    rendering_context->resources.command_pool.reset();
    rendering_context->resources.scene_data_buffer = nullptr;
}

void Renderer::draw_geometry(FrameContext& frame, const vk::raii::CommandBuffer& command_buffer)
{
    PORTAL_PROF_ZONE();

    const auto* rendering_context = std::any_cast<FrameRenderingContext>(&frame.rendering_context);

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
        command_buffer.beginRendering(render_target->make_rendering_info(*rendering_context));

        rendering_context->resources.scene_data_buffer = vulkan::BufferBuilder(sizeof(vulkan::GPUSceneData))
                                                         .with_usage(vk::BufferUsageFlagBits::eUniformBuffer)
                                                         .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                                                         .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                                                         .build(context.get_device());

        rendering_context->resources.scene_data_buffer.update_typed<vulkan::GPUSceneData>(rendering_context->scene_data);

        // TODO: support multiple global sets
        rendering_context->resources.global_descriptor_set = rendering_context->resources.frame_descriptors.allocate(
            scene_descriptor_set_layouts.front()
        );

        // TODO: create "global shader library" which will reflect the global changing data to write this
        vulkan::DescriptorWriter writer;
        writer.write_buffer(0, rendering_context->resources.scene_data_buffer, sizeof(vulkan::GPUSceneData), 0, vk::DescriptorType::eUniformBuffer);
        writer.update_set(context.get_device(), rendering_context->resources.global_descriptor_set);

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

                    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->get_vulkan_pipeline());
                    command_buffer.bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics,
                        pipeline->get_vulkan_pipeline_layout(),
                        0,
                        {rendering_context->resources.global_descriptor_set},
                        {}
                    );

                    command_buffer.setViewport(
                        0,
                        vk::Viewport(
                            0.0f,
                            0.0f,
                            static_cast<float>(swapchain->get_width()),
                            static_cast<float>(swapchain->get_height()),
                            0.0f,
                            1.0f
                        )
                    );
                    command_buffer.setScissor(
                        0,
                        vk::Rect2D(
                            vk::Offset2D(0, 0),
                            {static_cast<uint32_t>(swapchain->get_width()), static_cast<uint32_t>(swapchain->get_height())}
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

void Renderer::on_resize(const size_t new_width, const size_t new_height) const
{
    swapchain->on_resize(new_width, new_height);
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
} // portal
