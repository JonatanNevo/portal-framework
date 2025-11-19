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
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "vulkan/device/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/vulkan_pipeline.h"
#include "portal/engine/renderer/vulkan/vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"

#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/input/input_events.h"

namespace portal
{

[[maybe_unused]] static auto logger = Log::get_logger("Renderer");

Renderer::Renderer(Input& input, renderer::vulkan::VulkanContext& context, const Reference<renderer::vulkan::VulkanSwapchain>& swapchain)
    :
    swapchain(swapchain),
    context(context),
    renderer_context(context, render_target, scene_descriptor_set_layouts),
    camera(input)
{
    PORTAL_PROF_ZONE();

    make_render_target();
    camera.on_resize(static_cast<uint32_t>(swapchain->get_width()), static_cast<uint32_t>(swapchain->get_height()));
    init_descriptors();

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

        for (auto& f : frame_data)
        {
            f.deletion_queue.flush();
        }

        deletion_queue.flush();

        frame_data.clear();
        scene_descriptor_set_layouts.clear();
        render_target = nullptr;
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
    return swapchain->get_frames_in_flight();
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

FrameData& Renderer::get_current_frame_data()
{
    ZoneScoped;
    return frame_data[swapchain->get_current_frame()];
}

void Renderer::update_scene([[maybe_unused]] float delta_time, ResourceReference<Scene>& scene)
{
    ZoneScoped;
    auto start = std::chrono::system_clock::now();
    draw_context.render_objects.clear();
    camera.update(delta_time);

    const glm::mat4 view = camera.get_view();
    glm::mat4 projection = camera.get_projection();
    // invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
    projection[1][1] *= -1;

    scene_data.view = view;
    scene_data.proj = projection;
    scene_data.view_proj = projection * view;

    scene->draw(glm::mat4{1.f}, draw_context);

    //some default lighting parameters
    scene_data.ambient_color = glm::vec4(.1f);
    scene_data.sunlight_color = glm::vec4(1.f);
    scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);

    auto end = std::chrono::system_clock::now();

    //convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.scene_update_time = elapsed.count() / 1000.f;
}

void Renderer::begin_frame()
{
    PORTAL_PROF_ZONE();
    swapchain->begin_frame();

    // Resets descriptor pools
    clean_frame();

    const auto& current_command_buffer = swapchain->get_current_draw_command_buffer();
    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    current_command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

void Renderer::end_frame()
{
    const auto& current_command_buffer = swapchain->get_current_draw_command_buffer();
    current_command_buffer.end();
    // TODO: should we submit to graphics queue here?

    swapchain->present();
}

void Renderer::clean_frame()
{
    auto& current_frame = get_current_frame_data();
    current_frame.global_descriptor_set = nullptr;
    current_frame.frame_descriptors.clear_pools();
}

void Renderer::draw_geometry()
{
    auto& current_command_buffer = swapchain->get_current_draw_command_buffer();

    const auto draw_image = reference_cast<renderer::vulkan::VulkanImage>(render_target->get_image(0));
    const auto depth_image = reference_cast<renderer::vulkan::VulkanImage>(render_target->get_depth_image());

    {
        vulkan::transition_image_layout(
            current_command_buffer,
            draw_image->get_image_info().image.get_handle(),
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
            current_command_buffer,
            depth_image->get_image_info().image.get_handle(),
            1,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AccessFlagBits2::eMemoryWrite,
            vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::ImageAspectFlagBits::eDepth
            );
    }

    draw_geometry(current_command_buffer);

    {
        //transition the draw image and the swapchain image into their correct transfer layouts
        vulkan::transition_image_layout(
            current_command_buffer,
            draw_image->get_image_info().image.get_handle(),
            1,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eTransferRead,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::ImageAspectFlagBits::eColor
            );
        vulkan::transition_image_layout(
            current_command_buffer,
            swapchain->get_current_draw_image(),
            1,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::AccessFlagBits2::eMemoryWrite,
            vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::ImageAspectFlagBits::eColor
            );

        // execute a copy from the draw image into the swapchain
        vulkan::copy_image_to_image(
            current_command_buffer,
            draw_image->get_image_info().image.get_handle(),
            swapchain->get_current_draw_image(),
            {static_cast<uint32_t>(draw_image->get_width()), static_cast<uint32_t>(draw_image->get_height())},
            {static_cast<uint32_t>(swapchain->get_width()), static_cast<uint32_t>(swapchain->get_height())}
            );
    }
}


void Renderer::draw_geometry(const vk::raii::CommandBuffer& command_buffer)
{
    ZoneScoped;
    //reset counters
    stats.drawcall_count = 0;
    stats.triangle_count = 0;
    //begin clock
    auto start = std::chrono::system_clock::now();
    auto& current_frame = get_current_frame_data();

    auto current_frame_index = swapchain->get_current_frame();

    std::unordered_map<StringId, std::vector<uint32_t>> render_by_material;
    render_by_material.reserve(draw_context.render_objects.size());

    for (uint32_t i = 0; i < draw_context.render_objects.size(); i++)
    {
        const auto& object = draw_context.render_objects[i];
        if (object.is_visible(scene_data.view_proj))
            render_by_material[object.material->get_id()].push_back(i);
    }

    auto vulkan_target = reference_cast<renderer::vulkan::VulkanRenderTarget>(render_target);

    {
        command_buffer.beginRendering(vulkan_target->get_rendering_info());

        current_frame.scene_data_buffer = renderer::vulkan::BufferBuilder(sizeof(vulkan::GPUSceneData))
                                          .with_usage(vk::BufferUsageFlagBits::eUniformBuffer)
                                          .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                                          .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                                          .build(context.get_device());
        current_frame.scene_data_buffer.update_typed<vulkan::GPUSceneData>(scene_data);

        // TODO: support multiple global sets
        current_frame.global_descriptor_set = current_frame.frame_descriptors.allocate(scene_descriptor_set_layouts.front());

        // TODO: create "global shader library" which will reflect the global changing data to write this
        vulkan::DescriptorWriter writer;
        writer.write_buffer(0, current_frame.scene_data_buffer, sizeof(vulkan::GPUSceneData), 0, vk::DescriptorType::eUniformBuffer);
        writer.update_set(context.get_device(), current_frame.global_descriptor_set);

        Reference<renderer::vulkan::VulkanPipeline> last_pipeline = nullptr;
        Reference<renderer::vulkan::VulkanMaterial> last_material = nullptr;
        // MaterialPipeline* real_p = nullptr;
        std::shared_ptr<renderer::vulkan::AllocatedBuffer> last_index_buffer = nullptr;

        auto draw_object = [&](const scene::RenderObject& object)
        {
            // TracyVkZone(tracy_context, *current_frame.command_buffer, "Draw object");
            auto material = reference_cast<renderer::vulkan::VulkanMaterial>(object.material);
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
                        {current_frame.global_descriptor_set},
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
                        vk::Rect2D(vk::Offset2D(0, 0), {static_cast<uint32_t>(swapchain->get_width()), static_cast<uint32_t>(swapchain->get_height())})
                        );
                }

                const auto descriptor_set = material->get_descriptor_set(current_frame_index);
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
                draw_object(draw_context.render_objects[index]);
            }
        }

        command_buffer.endRendering();
    }

    current_frame.deletion_queue.push_deleter(
        [&current_frame]()
        {
            current_frame.scene_data_buffer = nullptr;
            current_frame.global_descriptor_set = nullptr;
        }
        );

    auto end = std::chrono::system_clock::now();

    //convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.mesh_draw_time = elapsed.count() / 1000.f;
}

void Renderer::on_resize(const size_t new_width, const size_t new_height)
{
    swapchain->on_resize(new_width, new_height);
    render_target->resize(new_width, new_height, true);
    camera.on_resize(static_cast<uint32_t>(new_width), static_cast<uint32_t>(new_height));
}


void Renderer::make_render_target()
{
    renderer::render_target::Specification spec{
        .width = swapchain->get_width(),
        .height = swapchain->get_height(),
        .attachments = {
            .attachments = {
                {
                    .format = renderer::ImageFormat::RGBA16_Float,
                    .blend = false
                },
                {
                    .format = renderer::ImageFormat::Depth_32Float,
                    .blend = true,
                    .blend_mode = renderer::render_target::BlendMode::Additive
                }
            }
        },
        .blend = true,
        .transfer = true,
        .name = STRING_ID("root_render_target")
    };
    render_target = std::make_shared<renderer::vulkan::VulkanRenderTarget>(spec, context);
}

void Renderer::init_descriptors()
{
    ZoneScoped;
    // create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<vulkan::DescriptorAllocator::PoolSizeRatio> sizes =
    {
        {vk::DescriptorType::eStorageImage, 1}
    };

    frame_data.clear();
    frame_data.reserve(swapchain->get_frames_in_flight());

    for (size_t i = 0; i < swapchain->get_frames_in_flight(); ++i)
    {
        std::vector<vulkan::DescriptorAllocator::PoolSizeRatio> frame_sizes = {
            {vk::DescriptorType::eStorageImage, 3},
            {vk::DescriptorType::eStorageBuffer, 3},
            {vk::DescriptorType::eUniformBuffer, 3},
            {vk::DescriptorType::eCombinedImageSampler, 4},
        };;

        frame_data.emplace_back(
            FrameData{
                .frame_descriptors = vulkan::DescriptorAllocator(context.get_device().get_handle(), 1000, frame_sizes)
            }
            );

        deletion_queue.push_deleter(
            [&, i]
            {
                frame_data[i].frame_descriptors.destroy_pools();
            }
            );
    }

    {
        vulkan::DescriptorLayoutBuilder builder;
        builder.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
        scene_descriptor_set_layouts.push_back(builder.build(context.get_device().get_handle()));
    }
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
