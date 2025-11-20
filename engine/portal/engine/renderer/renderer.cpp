//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

// #define VMA_STATIC_VULKAN_FUNCTIONS 0
// #define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
// #define VMA_IMPLEMENTATION
// #include "vma/vk_mem_alloc.h"

#include "renderer.h"

#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

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

Renderer::Renderer(Input& input, vulkan::VulkanContext& context, const Reference<vulkan::VulkanSwapchain>& swapchain)
    :
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
    renderer_context(context, scene_descriptor_set_layouts, attachments),
    camera(input)
{
    PORTAL_PROF_ZONE();

    camera.on_resize(static_cast<uint32_t>(swapchain->get_width()), static_cast<uint32_t>(swapchain->get_height()));
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
    ZoneScoped;
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

const RendererContext& Renderer::get_renderer_context() const
{
    return renderer_context;
}

size_t Renderer::get_frames_in_flight() const
{
    return frames_in_flight;
}

const renderer::vulkan::VulkanSwapchain& Renderer::get_swapchain() const
{
    return *swapchain;
}

void Renderer::on_event(Event& event)
{
    EventRunner runner(event);
    runner.run_on<KeyPressedEvent>(
        [&](const auto& e)
        {
            camera.on_key_down(e.get_key());
            return false;
        }
    );
    runner.run_on<KeyReleasedEvent>(
        [&](const auto& e)
        {
            camera.on_key_up(e.get_key());
            return false;
        }
    );
    runner.run_on<MouseMovedEvent>(
        [&](const auto& e)
        {
            camera.on_mouse_move(e.get_position());
            return false;
        }
    );
}

void Renderer::update_frame_time(const float frame_time)
{
    stats.frame_time = frame_time;
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
    ZoneScoped;

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

void Renderer::update_imgui([[maybe_unused]] float delta_time)
{
    ImGui::Begin("Stats");
    ImGui::Text("frametime %f ms", stats.frame_time);
    ImGui::Text("draw time %f ms", stats.mesh_draw_time);
    ImGui::Text("update time %f ms", stats.scene_update_time);
    ImGui::Text("triangles %i", stats.triangle_count);
    ImGui::Text("draws %i", stats.drawcall_count);
    ImGui::End();

    ImGui::Begin("Camera");
    ImGui::Text("position %f %f %f", camera.get_position().x, camera.get_position().y, camera.get_position().z);
    ImGui::Text("direction %f %f %f", camera.get_direction().x, camera.get_direction().y, camera.get_direction().z);

    // Input to control camera speed
    float camera_speed = camera.get_speed();
    if (ImGui::SliderFloat("Camera Speed", &camera_speed, 0.1f, 10.0f))
    {
        camera.set_speed(camera_speed);
    }
    ImGui::End();
}

void Renderer::update_scene(FrameContext& frame, ResourceReference<Scene>& scene)
{
    ZoneScoped;
    auto start = std::chrono::system_clock::now();
    camera.update(frame.delta_time);

    const glm::mat4 view = camera.get_view();
    glm::mat4 projection = camera.get_projection();
    // invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
    projection[1][1] *= -1;

    scene_data.view = view;
    scene_data.proj = projection;
    scene_data.view_proj = projection * view;

    scene->draw(glm::mat4{1.f}, frame);

    //some default lighting parameters
    scene_data.ambient_color = glm::vec4(.1f);
    scene_data.sunlight_color = glm::vec4(1.f);
    scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);

    auto end = std::chrono::system_clock::now();

    //convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.scene_update_time = elapsed.count() / 1000.f;
}

FrameContext Renderer::begin_frame()
{
    PORTAL_PROF_ZONE();

    // TODO: this allocates a new vector for each frame, use some cache instead
    FrameContext frame{
        .frame_index = current_frame,
        .depth_image = depth_image->get_image_info().image.get_handle(),
        .depth_image_view = depth_image->get_image_info().view,
        .command_buffer = frames[current_frame].command_buffer,
        .resources = frames[current_frame],
    };

    const auto& [image, image_view, last_used_frame] = swapchain->begin_frame(frame);

    frame.draw_image = image;
    frame.draw_image_view = image_view;

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
    frame.command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    return std::move(frame);
}

void Renderer::end_frame(const FrameContext& frame)
{
    frame.command_buffer.end();
    // TODO: should we submit to graphics queue here?

    swapchain->present(frame);

    current_frame = (current_frame + 1) % frames_in_flight;
}

void Renderer::clean_frame(const FrameContext& frame)
{
    frame.resources.deletion_queue.flush();
    frame.resources.global_descriptor_set = nullptr;
    frame.resources.frame_descriptors.clear_pools();
    frame.resources.command_pool.reset();
    frame.resources.scene_data_buffer = nullptr;
}

void Renderer::draw_geometry(FrameContext& frame)
{
    const auto draw_image = frame.draw_image;
    const auto depth_draw_image = frame.depth_image;

    vulkan::transition_image_layout(
        frame.command_buffer,
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
        frame.command_buffer,
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

    draw_geometry(frame, frame.command_buffer);

    // set draw image layout to Present so we can present it
    vulkan::transition_image_layout(
        frame.command_buffer,
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


void Renderer::draw_geometry(FrameContext& frame, const vk::raii::CommandBuffer& command_buffer)
{
    ZoneScoped;
    //reset counters
    stats.drawcall_count = 0;
    stats.triangle_count = 0;
    //begin clock
    auto start = std::chrono::system_clock::now();

    std::unordered_map<StringId, std::vector<uint32_t>> render_by_material;
    render_by_material.reserve(frame.render_objects.size());

    for (uint32_t i = 0; i < frame.render_objects.size(); i++)
    {
        const auto& object = frame.render_objects[i];
        if (object.is_visible(scene_data.view_proj))
            render_by_material[object.material->get_id()].push_back(i);
    }

    {
        command_buffer.beginRendering(render_target->make_rendering_info(frame));

        frame.resources.scene_data_buffer = vulkan::BufferBuilder(sizeof(vulkan::GPUSceneData))
                                            .with_usage(vk::BufferUsageFlagBits::eUniformBuffer)
                                            .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                                            .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                                            .build(context.get_device());
        frame.resources.scene_data_buffer.update_typed<vulkan::GPUSceneData>(scene_data);

        // TODO: support multiple global sets
        frame.resources.global_descriptor_set = frame.resources.frame_descriptors.allocate(scene_descriptor_set_layouts.front());

        // TODO: create "global shader library" which will reflect the global changing data to write this
        vulkan::DescriptorWriter writer;
        writer.write_buffer(0, frame.resources.scene_data_buffer, sizeof(vulkan::GPUSceneData), 0, vk::DescriptorType::eUniformBuffer);
        writer.update_set(context.get_device(), frame.resources.global_descriptor_set);

        Reference<vulkan::VulkanPipeline> last_pipeline = nullptr;
        Reference<vulkan::VulkanMaterial> last_material = nullptr;
        // MaterialPipeline* real_p = nullptr;
        std::shared_ptr<vulkan::AllocatedBuffer> last_index_buffer = nullptr;

        auto draw_object = [&](const RenderObject& object)
        {
            // TracyVkZone(tracy_context, *current_frame.command_buffer, "Draw object");
            auto material = reference_cast<vulkan::VulkanMaterial>(object.material);
            auto pipeline = material->get_pipeline();
            if (material != last_material)
            {
                last_material = material;
                //rebind pipeline and descriptors if the material changed
                if (pipeline != last_pipeline)
                {
                    last_pipeline = pipeline;

                    // if (pipeline->id == STRING_ID("color_pipeline"))
                    //     real_p = &metal_rough_material.opaque_pipeline;
                    // else if (pipeline->id == STRING_ID("transparent_pipeline"))
                    //     real_p = &metal_rough_material.transparent_pipeline;

                    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->get_vulkan_pipeline());
                    command_buffer.bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics,
                        pipeline->get_vulkan_pipeline_layout(),
                        0,
                        {frame.resources.global_descriptor_set},
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
            stats.drawcall_count++;
            stats.triangle_count += static_cast<uint32_t>(object.index_count / 3);
        };

        for (const auto& indexes : render_by_material | std::views::values)
        {
            for (const auto& index : indexes)
            {
                draw_object(frame.render_objects[index]);
            }
        }

        command_buffer.endRendering();
    }

    auto end = std::chrono::system_clock::now();

    //convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.mesh_draw_time = elapsed.count() / 1000.f;
}

void Renderer::on_resize(const size_t new_width, const size_t new_height)
{
    swapchain->on_resize(new_width, new_height);
    camera.on_resize(static_cast<uint32_t>(new_width), static_cast<uint32_t>(new_height));
}

void Renderer::immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function)
{
    ZoneScoped;
    context.get_device().get_handle().resetFences({immediate_fence});
    immediate_command_buffer.reset();

    immediate_command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    {
        TracyVkZone(tracy_context, *immediate_command_buffer, "Immediate Command Buffer");
        function(immediate_command_buffer);
    }
    TracyVkCollect(tracy_context, *immediate_command_buffer);
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
