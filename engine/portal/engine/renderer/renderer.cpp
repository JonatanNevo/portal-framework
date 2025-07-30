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

#include "portal/engine/renderer/vulkan_init.h"
#include "portal/engine/renderer/vulkan_utils.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/renderer/pipelines.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "portal/engine/renderer/descriptor_writer.h"
#include "portal/engine/renderer/pipeline_builder.h"
#include "portal/engine/renderer/base/allocated.h"
#include "portal/engine/renderer/scene/gltf_scene.h"

namespace portal
{

static auto logger = Log::get_logger("Renderer");

#define VK_HANDLE_CAST(raii_obj) reinterpret_cast<uint64_t>(static_cast<decltype(raii_obj)::CType>(*(raii_obj)))

#ifndef PORTAL_DIST
constexpr bool enable_validation_layers = true;
#else
constexpr bool enable_validation_layers = false;
#endif


static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* data
    )
{
    auto level = Log::LogLevel::Info;
    switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity))
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        level = Log::LogLevel::Debug;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        level = Log::LogLevel::Info;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        level = Log::LogLevel::Warn;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        level = Log::LogLevel::Error;
        break;
    }

    logger->log(SOURCE_LOC, static_cast<spdlog::level::level_enum>(level), pCallbackData->pMessage);
    return vk::False;
}


static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT extension,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* data
    )
{
    return debug_callback(
        static_cast<VkDebugUtilsMessageSeverityFlagBitsEXT>(severity),
        static_cast<VkDebugUtilsMessageTypeFlagsEXT>(extension),
        reinterpret_cast<const VkDebugUtilsMessengerCallbackDataEXT*>(pCallbackData),
        data
        );
}

void Renderer::init(const RendererSettings& renderer_settings)
{
    ZoneScoped;
    settings = renderer_settings;
    init_window();

    init_vulkan();
    init_swap_chain();
    init_commands();
    init_sync_structures();
    init_descriptors();
    init_pipelines();

    init_imgui();

    init_default_data();

    auto structure_file = vulkan::load_gltf(device, "resources/structure.glb", this);
    assert(structure_file);
    loaded_scenes["structure"] = structure_file.value();

    is_initialized = true;
}

void Renderer::run()
{
    ZoneScoped;
    timer.start();
    while (!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::system_clock::now();
        FrameMark;
        ZoneScopedN("Frame Loop");
        glfwPollEvents();

        if (resize_requested)
            resize_swap_chain();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Stats");
        ImGui::Text("frametime %f ms", stats.frame_time);
        ImGui::Text("draw time %f ms", stats.mesh_draw_time);
        ImGui::Text("update time %f ms", stats.scene_update_time);
        ImGui::Text("triangles %i", stats.triangle_count);
        ImGui::Text("draws %i", stats.drawcall_count);
        ImGui::End();

        ImGui::Render();
        draw(timer.tick<Timer::Seconds>());

        //get clock again, compare with start clock
        auto end = std::chrono::system_clock::now();

        //convert to microseconds (integer), and then come back to miliseconds
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        stats.frame_time = elapsed.count() / 1000.f;
    }
    device.waitIdle();
}

void Renderer::cleanup()
{
    ZoneScoped;
    if (is_initialized)
    {
        device.waitIdle();

        loaded_scenes.clear();

        for (auto& f : frame_data)
        {
            ZoneScopedN("FrameData Cleanup");
            f.deletion_queue.flush();
            TracyVkDestroy(f.tracy_context);
        }
        TracyVkDestroy(tracy_context);

        deletion_queue.flush();

        frame_data.clear();

        vulkan::allocation::shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    is_initialized = false;
}

FrameData& Renderer::get_current_frame_data()
{
    ZoneScoped;
    return frame_data[frame_number % MAX_FRAMES_IN_FLIGHT];
}

void Renderer::update_scene([[maybe_unused]] float delta_time)
{
    ZoneScoped;
    auto start = std::chrono::system_clock::now();
    draw_context.opaque_surfaces.clear();
    draw_context.transparent_surfaces.clear();
    camera.update(delta_time, window);

    const glm::mat4 view = camera.get_view();
    glm::mat4 projection = camera.get_projection();
    // invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
    projection[1][1] *= -1;

    scene_data.view = view;
    scene_data.proj = projection;
    scene_data.view_proj = projection * view;

    loaded_scenes["structure"]->draw(glm::mat4{1.f}, draw_context);


    //some default lighting parameters
    scene_data.ambient_color = glm::vec4(.1f);
    scene_data.sunlight_color = glm::vec4(1.f);
    scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);

    auto end = std::chrono::system_clock::now();

    //convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.scene_update_time = elapsed.count() / 1000.f;
}

vulkan::GPUMeshBuffers Renderer::upload_mesh(std::span<uint32_t> indices, std::span<vulkan::Vertex> vertices)
{
    ZoneScoped;
    const size_t vertex_buffer_size = vertices.size() * sizeof(vulkan::Vertex);
    const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

    vulkan::BufferBuilder vertex_builder{vertex_buffer_size};
    vertex_builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                  .with_usage(
                      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress
                      )
                  .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

    vulkan::BufferBuilder index_builder{index_buffer_size};
    index_builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                 .with_usage(
                     vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
                     )
                 .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);


    vulkan::GPUMeshBuffers buffers{
        .index_buffer = index_builder.build(device),
        .vertex_buffer = vertex_builder.build(device),
    };
    buffers.vertex_buffer_address = buffers.vertex_buffer.get_device_address();

    auto staging = vulkan::AllocatedBuffer::create_staging_buffer(device, vertex_buffer_size + index_buffer_size, nullptr);
    auto offset = staging.update(vertices.data(), vertex_buffer_size, 0);
    offset += staging.update(indices.data(), index_buffer_size, offset);

    immediate_submit(
        [&](const vk::raii::CommandBuffer& command_buffer)
        {
            vk::BufferCopy vertex_copy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = vertex_buffer_size
            };
            command_buffer.copyBuffer(
                staging.get_handle(),
                buffers.vertex_buffer.get_handle(),
                {vertex_copy}
                );

            vk::BufferCopy index_copy{
                .srcOffset = vertex_buffer_size,
                .dstOffset = 0,
                .size = index_buffer_size
            };
            command_buffer.copyBuffer(
                staging.get_handle(),
                buffers.index_buffer.get_handle(),
                {index_copy}
                );
        }
        );

    return buffers;
}

void Renderer::draw([[maybe_unused]] float delta_time)
{
    ZoneScoped;
    update_scene(delta_time);

    auto& current_frame = get_current_frame_data();

    {
        ZoneScopedN("gpu wait");
        // wait until the gpu has finished rendering the last frame. Timeout of 1 second
        while (device.waitForFences({current_frame.render_fence}, true, 1000000000) == vk::Result::eTimeout)
        {
            // Wait
        }
    }

    current_frame.deletion_queue.flush();
    current_frame.frame_descriptors.clear_pools();
    unsigned image_index = 0;

    {
        ZoneScopedN("acquire Image");
        auto [result, index] = swap_chain.acquireNextImage(
            1000000000,
            current_frame.swap_chain_semaphore,
            nullptr
            );
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            resize_requested = true;
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        image_index = index;
    }

    draw_extent.width = static_cast<uint32_t>(std::min(swap_chain_extent.width, draw_image.get_extent().width) * render_scale);
    draw_extent.height = static_cast<uint32_t>(std::min(swap_chain_extent.height, draw_image.get_extent().height) * render_scale);

    device.resetFences({current_frame.render_fence});

    current_frame.command_buffer.reset();

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    current_frame.command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    {
        ZoneScopedN("command recoding");
        TracyVkZone(tracy_context, *current_frame.command_buffer, "Draw");

        {
            TracyVkZone(tracy_context, *current_frame.command_buffer, "Initial Image Transitions");
            vulkan::transition_image_layout(
                current_frame.command_buffer,
                draw_image.get_handle(),
                1,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::ImageAspectFlagBits::eColor
                );
            vulkan::transition_image_layout(
                current_frame.command_buffer,
                depth_image.get_handle(),
                1,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::ImageAspectFlagBits::eDepth
                );
        }

        {
            TracyVkZone(tracy_context, *current_frame.command_buffer, "Draw Geometry");
            draw_geometry(current_frame.command_buffer);
        }

        {
            TracyVkZone(tracy_context, *current_frame.command_buffer, "Post Image Transitions");
            //transition the draw image and the swapchain image into their correct transfer layouts
            vulkan::transition_image_layout(
                current_frame.command_buffer,
                draw_image.get_handle(),
                1,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::ImageAspectFlagBits::eColor
                );
            vulkan::transition_image_layout(
                current_frame.command_buffer,
                swap_chain_images[image_index],
                1,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::ImageAspectFlagBits::eColor
                );
        }

        {
            TracyVkZone(tracy_context, *current_frame.command_buffer, "IMGUI");
            // execute a copy from the draw image into the swapchain
            vulkan::copy_image_to_image(
                current_frame.command_buffer,
                draw_image.get_handle(),
                swap_chain_images[image_index],
                draw_extent,
                swap_chain_extent
                );

            // set swapchain image layout to Attachment Optimal so we can draw it
            vulkan::transition_image_layout(
                current_frame.command_buffer,
                swap_chain_images[image_index],
                1,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::ImageAspectFlagBits::eColor
                );

            //draw imgui into the swapchain image
            draw_imgui(current_frame.command_buffer, swap_chain_image_views[image_index]);

            // set swapchain image layout to Present so we can draw it
            vulkan::transition_image_layout(
                current_frame.command_buffer,
                swap_chain_images[image_index],
                1,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::ePresentSrcKHR,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::ImageAspectFlagBits::eColor
                );
        }
    }
    TracyVkCollect(tracy_context, *current_frame.command_buffer);
    current_frame.command_buffer.end();

    {
        ZoneScopedN("Present");
        vk::PipelineStageFlags wait_destination_stage_mask = vk::PipelineStageFlagBits::eAllCommands;
        const vk::SubmitInfo submit_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*current_frame.swap_chain_semaphore,
            .pWaitDstStageMask = &wait_destination_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*current_frame.command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*current_frame.render_semaphore,
        };
        graphics_queue.submit(submit_info, *current_frame.render_fence);

        const vk::PresentInfoKHR present_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*current_frame.render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*swap_chain,
            .pImageIndices = &image_index
        };

        try
        {
            const auto result = present_queue.presentKHR(present_info);
            if (result == vk::Result::eSuboptimalKHR)
            {
                resize_requested = true;
            }
        }
        catch (vk::OutOfDateKHRError&)
        {
            resize_requested = true;
        }
    }
    frame_number++;
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

    std::vector<uint32_t> opaque_draws;
    opaque_draws.reserve(draw_context.opaque_surfaces.size());

    for (uint32_t i = 0; i < draw_context.opaque_surfaces.size(); i++)
    {
        const auto& object = draw_context.opaque_surfaces[i];
        if (object.is_visible(scene_data.view_proj))
            opaque_draws.push_back(i);
    }

    // sort the opaque surfaces by material and mesh
    std::ranges::sort(
        opaque_draws,
        [&](const auto& iA, const auto& iB)
        {
            const RenderObject& A = draw_context.opaque_surfaces[iA];
            const RenderObject& B = draw_context.opaque_surfaces[iB];
            if (A.material == B.material)
            {
                return A.index_buffer < B.index_buffer;
            }
            return A.material < B.material;
        }
        );


    vk::RenderingAttachmentInfo color_attachment{
        .imageView = draw_image.get_view(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
    };

    vk::RenderingAttachmentInfo depth_attachment{
        .imageView = depth_image.get_view(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearDepthStencilValue(0, 0)
    };

    const vk::RenderingInfo rendering_info{
        .renderArea = vk::Rect2D({0, 0}, draw_extent),
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
        .pStencilAttachment = nullptr
    };

    {
        command_buffer.beginRendering(rendering_info);

        current_frame.scene_data_buffer = vulkan::BufferBuilder(sizeof(vulkan::GPUSceneData))
                                          .with_usage(vk::BufferUsageFlagBits::eUniformBuffer)
                                          .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                                          .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                                          .build(device);
        current_frame.scene_data_buffer.update_typed<vulkan::GPUSceneData>(scene_data);

        current_frame.global_descriptor_set = current_frame.frame_descriptors.allocate(scene_descriptor_set_layout);

        vulkan::DescriptorWriter writer;
        writer.write_buffer(0, current_frame.scene_data_buffer, sizeof(vulkan::GPUSceneData), 0, vk::DescriptorType::eUniformBuffer);
        writer.update_set(device, current_frame.global_descriptor_set);

        MaterialPipeline* last_pipeline = nullptr;
        MaterialInstance* last_material = nullptr;
        vulkan::AllocatedBuffer* last_index_buffer = nullptr;

        auto draw_object = [&](const RenderObject& object)
        {
            TracyVkZone(tracy_context, *current_frame.command_buffer, "Draw object");
            auto& [pipeline, layout] = *object.material->pipeline;
            if (object.material != last_material)
            {
                last_material = object.material;
                //rebind pipeline and descriptors if the material changed
                if (object.material->pipeline != last_pipeline)
                {
                    last_pipeline = object.material->pipeline;
                    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
                    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, {current_frame.global_descriptor_set}, {});

                    command_buffer.setViewport(
                        0,
                        vk::Viewport(
                            0.0f,
                            0.0f,
                            static_cast<float>(window_extent.width),
                            static_cast<float>(window_extent.height),
                            0.0f,
                            1.0f
                            )
                        );
                    command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), window_extent));
                }
                command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 1, {object.material->material_set}, {});
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
            command_buffer.pushConstants<vulkan::GPUDrawPushConstants>(layout, vk::ShaderStageFlagBits::eVertex, 0, {push_constants});

            command_buffer.drawIndexed(object.index_count, 1, object.first_index, 0, 0);

            //add counters for triangles and draws
            stats.drawcall_count++;
            stats.triangle_count += object.index_count / 3;
        };

        for (const auto& index : opaque_draws)
        {
            draw_object(draw_context.opaque_surfaces[index]);
        }

        for (const auto& obj : draw_context.transparent_surfaces)
        {
            draw_object(obj);
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

void Renderer::draw_imgui(const vk::raii::CommandBuffer& command_buffer, const vk::raii::ImageView& target_image_view) const
{
    ZoneScoped;
    vk::RenderingAttachmentInfo color_attachment = {
        .imageView = target_image_view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore
    };
    const vk::RenderingInfo rendering_info = {
        .renderArea = vk::Rect2D({0, 0}, swap_chain_extent),
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr
    };

    command_buffer.beginRendering(rendering_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *command_buffer);
    command_buffer.endRendering();
}

void Renderer::resize_swap_chain()
{
    ZoneScoped;

    // Add additional safety checks
    if (!is_initialized || device == nullptr)
    {
        LOGGER_WARN("Attempted to resize swap chain before renderer is initialized");
        return;
    }

    device.waitIdle();

    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Clear swap chain images before destroying swap chain to avoid use-after-free
    swap_chain_image_views.clear();
    swap_chain_images.clear();
    swap_chain = nullptr;

    window_extent = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    create_swap_chain(width, height);

    camera.on_resize(width, height);
    resize_requested = false;
}

void Renderer::init_window()
{
    ZoneScoped;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (settings.height == -1 || settings.width == -1)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        window_extent.width = mode->width;
        window_extent.height = mode->height;
    }
    else
    {
        window_extent.width = settings.width;
        window_extent.height = settings.height;
    }

    window = glfwCreateWindow(window_extent.width, window_extent.height, settings.title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);

    // glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
    LOGGER_DEBUG("Created window with size: {}x{}", window_extent.width, window_extent.height);
}

void Renderer::init_imgui()
{
    ZoneScoped;
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo itself.
    const vk::DescriptorPoolSize pool_sizes[] = {
        {vk::DescriptorType::eSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eSampledImage, 1000},
        {vk::DescriptorType::eStorageImage, 1000},
        {vk::DescriptorType::eUniformTexelBuffer, 1000},
        {vk::DescriptorType::eStorageTexelBuffer, 1000},
        {vk::DescriptorType::eUniformBuffer, 1000},
        {vk::DescriptorType::eStorageBuffer, 1000},
        {vk::DescriptorType::eUniformBufferDynamic, 1000},
        {vk::DescriptorType::eStorageBufferDynamic, 1000},
        {vk::DescriptorType::eInputAttachment, 1000}
    };

    vk::DescriptorPoolCreateInfo pool_info = {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000,
        .poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes)),
        .pPoolSizes = pool_sizes
    };

    auto imgui_pool = (*device).createDescriptorPool(pool_info);

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = *instance,
        .PhysicalDevice = *physical_device,
        .Device = *device,
        .Queue = *graphics_queue,
        .DescriptorPool = imgui_pool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfoKHR{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swap_chain_image_format
        },
    };

    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();

    deletion_queue.push_deleter(
        [this, imgui_pool]()
        {
            ImGui_ImplVulkan_Shutdown();
            (*device).destroyDescriptorPool(imgui_pool);
        }
        );
}

void Renderer::init_vulkan()
{
    ZoneScoped;
    /// Create Instance
    {
        constexpr vk::ApplicationInfo app_info = {
            .pApplicationName = "Portal Engine",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Portal Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14
        };

        auto required_layers = vulkan::get_required_validation_layers(context, enable_validation_layers);
        auto required_extensions = vulkan::get_required_extensions(context, enable_validation_layers);

        const vk::InstanceCreateInfo create_info = {
#ifdef PORTAL_PLATFORM_MACOS
            .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif
            .pApplicationInfo = &app_info,
            .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
            .ppEnabledLayerNames = required_layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
            .ppEnabledExtensionNames = required_extensions.data()
        };
        instance = context.createInstance(create_info);
        LOGGER_DEBUG("Initialing Vulkan");
        LOGGER_TRACE(
            "Vulkan version: {}.{}.{}",
            VK_VERSION_MAJOR(vk::ApiVersion14),
            VK_VERSION_MINOR(vk::ApiVersion14),
            VK_VERSION_PATCH(vk::ApiVersion14)
            );
    }

    /// Create Debug Messenger
    if (enable_validation_layers)
    {
        constexpr auto severity_flags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        constexpr auto message_type_flags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

        constexpr vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create{
            .messageSeverity = severity_flags,
            .messageType = message_type_flags,
            .pfnUserCallback = &debug_callback,
        };
        debug_messenger = instance.createDebugUtilsMessengerEXT(debug_utils_create);
    }

    /// Create Surface
    {
        VkSurfaceKHR surface_handle;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &surface_handle) != VK_SUCCESS)
        {
            LOGGER_ERROR("Failed to create window surface!");
            throw std::runtime_error("Failed to create window surface!");
        }
        surface = vk::raii::SurfaceKHR(instance, surface_handle);
    }


    /// Choose Physical Device
    {
        const auto devices = instance.enumeratePhysicalDevices();
        if (devices.empty())
        {
            LOGGER_ERROR("No Vulkan physical devices found!");
            throw std::runtime_error("No Vulkan physical devices found!");
        }

        std::multimap<uint32_t, vk::raii::PhysicalDevice> candidates;
        for (const auto& dev : devices)
        {
            uint32_t score = vulkan::rate_device_suitability(dev);
            LOGGER_DEBUG("Gpu candidate: {} with score {}", dev.getProperties().deviceName.data(), score);
            candidates.insert(std::make_pair(score, dev));
        }

        // Check if the best candidate is suitable at all
        if (candidates.rbegin()->first > 0)
        {
            physical_device = candidates.rbegin()->second;
            msaa_samples = vulkan::get_max_usable_sample_count(physical_device);
        }
        else
            throw std::runtime_error("No suitable gpu found!");

        LOGGER_INFO("Picked GPU: {}", physical_device.getProperties().deviceName.data());
    }


    /// Create Logical Device
    {
        auto graphics_queue_family_index = vulkan::find_queue_families(physical_device, vk::QueueFlagBits::eGraphics);

        auto queue_families = physical_device.getQueueFamilyProperties();
        auto present_queue_family_index = physical_device.getSurfaceSupportKHR(graphics_queue_family_index, *surface)
            ? graphics_queue_family_index
            : queue_families.size();
        if (present_queue_family_index == queue_families.size())
        {
            // the graphicsIndex doesn't support present -> look for another family index that supports both
            // graphics and present
            for (size_t i = 0; i < queue_families.size(); i++)
            {
                if ((queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                    physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
                {
                    graphics_queue_family_index = static_cast<uint32_t>(i);
                    present_queue_family_index = graphics_queue_family_index;
                    break;
                }
            }
            if (present_queue_family_index == queue_families.size())
            {
                // there's nothing like a single family index that supports both graphics and present -> look for another
                // family index that supports present
                for (size_t i = 0; i < queue_families.size(); i++)
                {
                    if (physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
                    {
                        present_queue_family_index = static_cast<uint32_t>(i);
                        break;
                    }
                }
            }
        }
        if ((graphics_queue_family_index == queue_families.size()) || (present_queue_family_index == queue_families.size()))
        {
            LOGGER_ERROR("Could not find a queue family that supports graphics or present");
            throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
        }

        float queue_priority = 0.0f;
        vk::DeviceQueueCreateInfo queue_create_info = {
            .queueFamilyIndex = graphics_queue_family_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };


        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
            feature_chain = {
                {.features = {.sampleRateShading = true, .samplerAnisotropy = true}},
                {.bufferDeviceAddress = true},
                {.synchronization2 = true, .dynamicRendering = true},
                {.extendedDynamicState = true}
            };

        vk::DeviceCreateInfo create_info = {
            .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_create_info,
            .enabledExtensionCount = static_cast<uint32_t>(vulkan::g_device_extensions.size()),
            .ppEnabledExtensionNames = vulkan::g_device_extensions.data(),
        };
        device = physical_device.createDevice(create_info);

        graphics_queue = device.getQueue(graphics_queue_family_index, 0);
        graphics_family_index = graphics_queue_family_index;
        present_queue = device.getQueue(static_cast<uint32_t>(present_queue_family_index), 0);
        present_family_index = graphics_queue_family_index;
    }

    // Create allocator
    vulkan::allocation::init(instance, physical_device, device);
}

void Renderer::init_swap_chain()
{
    ZoneScoped;
    create_swap_chain(window_extent.width, window_extent.height);

    const vk::Extent2D draw_image_extent{
        .width = window_extent.width,
        .height = window_extent.height,
    };

    vulkan::ImageBuilder color_image_builder(draw_image_extent);
    draw_image = color_image_builder
                 .with_usage(
                     vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eColorAttachment
                     )
                 .with_format(vk::Format::eR16G16B16A16Sfloat)
                 .with_sample_count(vk::SampleCountFlagBits::e1)
                 .with_tiling(vk::ImageTiling::eOptimal)
                 .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                 .with_vma_required_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
                 .with_mips_levels(static_cast<uint32_t>(std::floor(std::log2(std::max(draw_image_extent.width, draw_image_extent.height)))) + 1)
                 .with_debug_name("draw_image")
                 .build(device);

    vulkan::ImageBuilder depth_image_builder(draw_image_extent);
    depth_image = depth_image_builder
                  .with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                  .with_format(vk::Format::eD32Sfloat)
                  .with_sample_count(vk::SampleCountFlagBits::e1)
                  .with_tiling(vk::ImageTiling::eOptimal)
                  .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                  .with_vma_required_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
                  .with_debug_name("depth_image")
                  .build(device);

    camera.on_resize(window_extent.width, window_extent.height);

    deletion_queue.push_deleter(
        [this]
        {
            draw_image = nullptr;
            depth_image = nullptr;
        }
        );
}

void Renderer::init_commands()
{
    ZoneScoped;
    for (auto& data : frame_data)
    {
        data.command_pool = vulkan::create_command_pool(device, graphics_family_index, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        data.command_buffer = vulkan::allocate_command_buffer(device, data.command_pool, vk::CommandBufferLevel::ePrimary);
        data.tracy_context = TracyVkContext(*physical_device, *device, *graphics_queue, *data.command_buffer);
        device.setDebugUtilsObjectNameEXT(
            {
                .objectType = vk::ObjectType::eCommandPool,
                .objectHandle = VK_HANDLE_CAST(data.command_pool),
                .pObjectName = "frame_command_pool"
            }
            );
        device.setDebugUtilsObjectNameEXT(
            {
                .objectType = vk::ObjectType::eCommandBuffer,
                .objectHandle = VK_HANDLE_CAST(data.command_buffer),
                .pObjectName = "frame_command_buffer"
            }
            );
    }

    immediate_command_pool = vulkan::create_command_pool(
        device,
        graphics_family_index,
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient
        );

    immediate_command_buffer = vulkan::allocate_command_buffer(device, immediate_command_pool, vk::CommandBufferLevel::ePrimary);
    tracy_context = TracyVkContext(*physical_device, *device, *graphics_queue, *immediate_command_buffer);

    device.setDebugUtilsObjectNameEXT(
        {
            .objectType = vk::ObjectType::eCommandPool,
            .objectHandle = VK_HANDLE_CAST(immediate_command_pool),
            .pObjectName = "immediate_command_pool"
        }
        );
    device.setDebugUtilsObjectNameEXT(
        {
            .objectType = vk::ObjectType::eCommandBuffer,
            .objectHandle = VK_HANDLE_CAST(immediate_command_buffer),
            .pObjectName = "immediate_command_buffer"
        }
        );

}

void Renderer::init_sync_structures()
{
    ZoneScoped;
    // create synchronization structures
    // one fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to synchronize rendering with swap chain
    // we want the fence to start signalled so we can wait on it on the first frame
    for (auto& data : frame_data)
    {
        data.swap_chain_semaphore = device.createSemaphore({});
        data.render_semaphore = device.createSemaphore({});
        data.render_fence = device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});
    }

    immediate_fence = device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});
}

void Renderer::init_descriptors()
{
    ZoneScoped;
    // create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<vulkan::DescriptorAllocator::PoolSizeRatio> sizes =
    {
        {vk::DescriptorType::eStorageImage, 1}
    };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        std::vector<vulkan::DescriptorAllocator::PoolSizeRatio> frame_sizes = {
            {vk::DescriptorType::eStorageImage, 3},
            {vk::DescriptorType::eStorageBuffer, 3},
            {vk::DescriptorType::eUniformBuffer, 3},
            {vk::DescriptorType::eCombinedImageSampler, 4},
        };

        frame_data[i].frame_descriptors = vulkan::DescriptorAllocator();
        frame_data[i].frame_descriptors.init(&device, 1000, frame_sizes);

        deletion_queue.push_deleter(
            [&, i]
            {
                frame_data[i].frame_descriptors.destroy_pools();
            }
            );
    }

    {
        vulkan::DescriptorLayoutBuilder builder;
        builder.add_binding(0, vk::DescriptorType::eUniformBuffer);
        scene_descriptor_set_layout = builder.build(device, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
    }
}

void Renderer::init_pipelines()
{
    ZoneScoped;
    metal_rough_material.build_pipelines(device, scene_descriptor_set_layout, draw_image.get_format(), depth_image.get_format());
}

void Renderer::init_default_data()
{
    ZoneScoped;
    //3 default textures, white, grey, black. 1 pixel each
    {
        const uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        vulkan::ImageBuilder builder(1, 1, 1);
        white_image = builder
                      .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc)
                      .with_format(vk::Format::eR8G8B8A8Unorm)
                      .with_tiling(vk::ImageTiling::eOptimal)
                      .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                      .build(device);
        populate_image(&white, white_image);
    }
    {
        const uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
        vulkan::ImageBuilder builder(1, 1, 1);
        grey_image = builder
                     .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc)
                     .with_format(vk::Format::eR8G8B8A8Unorm)
                     .with_tiling(vk::ImageTiling::eOptimal)
                     .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                     .build(device);
        populate_image(&grey, grey_image);
    }
    {
        const uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        vulkan::ImageBuilder builder(1, 1, 1);
        black_image = builder
                      .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc)
                      .with_format(vk::Format::eR8G8B8A8Unorm)
                      .with_tiling(vk::ImageTiling::eOptimal)
                      .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                      .build(device);
        populate_image(&black, black_image);
    }
    //checkerboard image
    {
        const uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        const uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));

        std::array<uint32_t, 16 * 16> pixels{}; //for 16x16 checkerboard texture
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        vulkan::ImageBuilder builder(16, 16, 1);
        error_checker_board_image = builder
                                    .with_usage(
                                        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
                                        )
                                    .with_format(vk::Format::eR8G8B8A8Unorm)
                                    .with_tiling(vk::ImageTiling::eOptimal)
                                    .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                                    .build(device);
        populate_image(pixels.data(), error_checker_board_image);
    }

    {
        vk::SamplerCreateInfo sampler_info{
            .magFilter = vk::Filter::eNearest,
            .minFilter = vk::Filter::eNearest,
        };
        default_sampler_nearest = device.createSampler(sampler_info);

        sampler_info.magFilter = vk::Filter::eLinear;
        sampler_info.minFilter = vk::Filter::eLinear;
        default_sampler_linear = device.createSampler(sampler_info);
    }

    camera.set_position(glm::vec3(30.f, -00.f, -085.f));

    deletion_queue.push_deleter(
        [this]
        {
            white_image = nullptr;
            grey_image = nullptr;
            black_image = nullptr;
            error_checker_board_image = nullptr;
            draw_context = {};
        }
        );
}

void Renderer::create_swap_chain(const uint32_t width, const uint32_t height)
{
    ZoneScoped;

    // Validate that required objects are still valid
    if (physical_device == nullptr || surface == nullptr || device == nullptr)
    {
        LOGGER_ERROR("Invalid Vulkan objects when creating swap chain");
        throw std::runtime_error("Invalid Vulkan objects when creating swap chain");
    }

    //Create swap chain
    {
        const auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);

        // Cache the surface formats and present modes to avoid multiple calls
        const auto surface_formats = physical_device.getSurfaceFormatsKHR(*surface);
        const auto present_modes = physical_device.getSurfacePresentModesKHR(*surface);

        const auto [format, colorSpace] = vulkan::choose_surface_format(surface_formats);
        swap_chain_extent = vulkan::choose_extent(width, height, surface_capabilities);
        const auto min_image_count = std::min(std::max(3u, surface_capabilities.minImageCount), surface_capabilities.maxImageCount);

        const vk::SwapchainCreateInfoKHR swap_chain_create_info = {
            .flags = vk::SwapchainCreateFlagsKHR(),
            .surface = surface,
            .minImageCount = min_image_count,
            .imageFormat = format,
            .imageColorSpace = colorSpace,
            .imageExtent = swap_chain_extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform = surface_capabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = vulkan::choose_present_mode(present_modes),
            .clipped = true,
            .oldSwapchain = nullptr
        };
        swap_chain = device.createSwapchainKHR(swap_chain_create_info);
        swap_chain_images = swap_chain.getImages();
        swap_chain_image_format = format;
    }

    // Create Image Views
    {
        swap_chain_image_views.clear();
        for (const auto image : swap_chain_images)
        {
            swap_chain_image_views.emplace_back(
                vulkan::create_image_view(device, image, 1, swap_chain_image_format, vk::ImageAspectFlagBits::eColor)
                );
        }
    }
}

void Renderer::populate_image(const void* data, vulkan::AllocatedImage& image)
{
    ZoneScoped;
    // TODO: get data size from format
    const size_t data_size = image.get_extent().width * image.get_extent().height * image.get_extent().depth * 4;
    auto staging_buffer = vulkan::AllocatedBuffer::create_staging_buffer(device, data_size, data);

    immediate_submit(
        [&](const auto& command_buffer)
        {
            TracyVkZone(tracy_context, *command_buffer, "populate image");
            vulkan::transition_image_layout(
                command_buffer,
                image.get_handle(),
                image.get_mip_levels(),
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal
                );

            vk::BufferImageCopy copy_region = {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .imageExtent = image.get_extent(),
            };

            command_buffer.copyBufferToImage(
                staging_buffer.get_handle(),
                image.get_handle(),
                vk::ImageLayout::eTransferDstOptimal,
                {copy_region}
                );

            generate_mipmaps(command_buffer, image);
        }
        );
}

void Renderer::generate_mipmaps(const vk::raii::CommandBuffer& command_buffer, const vulkan::AllocatedImage& image)
{
    TracyVkZone(tracy_context, *command_buffer, "generate mipmaps");
    const auto format_properties = physical_device.getFormatProperties(image.get_format());
    if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }

    int32_t mip_width = image.get_extent().width;
    int32_t mip_height = image.get_extent().height;

    vk::ImageSubresourceRange subresource_range = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };

    for (uint32_t i = 1; i < image.get_mip_levels(); ++i)
    {
        subresource_range.baseMipLevel = i - 1;
        vulkan::transition_image_layout(
            command_buffer,
            image.get_handle(),
            subresource_range,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::AccessFlagBits2::eTransferWrite,
            vk::AccessFlagBits2::eTransferRead,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::PipelineStageFlagBits2::eTransfer
            );;


        vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dst_offsets;
        offsets[0] = vk::Offset3D(0, 0, 0);
        offsets[1] = vk::Offset3D(mip_width, mip_height, 1);
        dst_offsets[0] = vk::Offset3D(0, 0, 0);
        dst_offsets[1] = vk::Offset3D(mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1);
        vk::ImageBlit blit = {
            .srcSubresource = {vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
            .srcOffsets = offsets,
            .dstSubresource = {vk::ImageAspectFlagBits::eColor, i, 0, 1},
            .dstOffsets = dst_offsets
        };
        command_buffer.blitImage(
            image.get_handle(),
            vk::ImageLayout::eTransferSrcOptimal,
            image.get_handle(),
            vk::ImageLayout::eTransferDstOptimal,
            {blit},
            vk::Filter::eLinear
            );

        vulkan::transition_image_layout(
            command_buffer,
            image.get_handle(),
            subresource_range,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::AccessFlagBits2::eTransferRead,
            vk::AccessFlagBits2::eShaderRead,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::PipelineStageFlagBits2::eFragmentShader
            );
        if (mip_width > 1)
            mip_width /= 2;
        if (mip_height > 1)
            mip_height /= 2;
    }

    subresource_range.baseMipLevel = image.get_mip_levels() - 1;
    vulkan::transition_image_layout(
        command_buffer,
        image.get_handle(),
        subresource_range,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite,
        vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::PipelineStageFlagBits2::eFragmentShader
        );

    // ZoneScoped;
    // const auto format_properties = physical_device.getFormatProperties(image.get_format());
    // if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    // {
    //     throw std::runtime_error("Texture image format does not support linear blitting!");
    // }
    //
    // vk::ImageMemoryBarrier2 barrier = {
    //     .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
    //     .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
    //     .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
    //     .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
    //     .oldLayout = vk::ImageLayout::eTransferDstOptimal,
    //     .newLayout = vk::ImageLayout::eTransferSrcOptimal,
    //     .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
    //     .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
    //     .image = image.get_handle(),
    //     .subresourceRange = {
    //         .aspectMask = vk::ImageAspectFlagBits::eColor,
    //         .baseMipLevel = 0,
    //         .levelCount = 1,
    //         .baseArrayLayer = 0,
    //         .layerCount = 1
    //     }
    // };
    //
    // int32_t mip_width = image.get_extent().width;
    // int32_t mip_height = image.get_extent().height;
    //
    // const vk::DependencyInfo dependency_info = {
    //     .dependencyFlags = {},
    //     .imageMemoryBarrierCount = 1,
    //     .pImageMemoryBarriers = &barrier
    // };
    // for (uint32_t i = 1; i < image.get_subresource().mipLevel; ++i)
    // {
    //     {
    //         TracyVkZone(tracy_context, *command_buffer, "mip initial transform");
    //         barrier.subresourceRange.baseMipLevel = i - 1;
    //         barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
    //             barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
    //             barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    //         barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    //         barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    //         barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
    //         command_buffer.pipelineBarrier2(dependency_info);
    //     }
    //
    //     {
    //         TracyVkZone(tracy_context, *command_buffer, "mip blit");
    //         vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dst_offsets;
    //         offsets[0] = vk::Offset3D(0, 0, 0);
    //         offsets[1] = vk::Offset3D(mip_width, mip_height, 1);
    //         dst_offsets[0] = vk::Offset3D(0, 0, 0);
    //         dst_offsets[1] = vk::Offset3D(mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1);
    //         vk::ImageBlit blit = {
    //             .srcSubresource = {vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
    //             .srcOffsets = offsets,
    //             .dstSubresource = {vk::ImageAspectFlagBits::eColor, i, 0, 1},
    //             .dstOffsets = dst_offsets
    //         };
    //         command_buffer.blitImage(
    //             image.get_handle(),
    //             vk::ImageLayout::eTransferSrcOptimal,
    //             image.get_handle(),
    //             vk::ImageLayout::eTransferDstOptimal,
    //             {blit},
    //             vk::Filter::eLinear
    //             );
    //     }
    //
    //
    //     {
    //         TracyVkZone(tracy_context, *command_buffer, "mip end transform");
    //         barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
    //             barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
    //             barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    //         barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    //         barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
    //         barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
    //         command_buffer.pipelineBarrier2(dependency_info);
    //
    //         if (mip_width > 1)
    //             mip_width /= 2;
    //         if (mip_height > 1)
    //             mip_height /= 2;
    //     }
    // }
    //
    // TracyVkZone(tracy_context, *command_buffer, "mip level last transform");
    //
    // barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
    //     barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
    //     barrier.subresourceRange.baseMipLevel = image.get_subresource().mipLevel - 1;
    // barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    // barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    // barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    // barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
    // command_buffer.pipelineBarrier2(dependency_info);
}

void Renderer::immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function)
{
    ZoneScoped;
    device.resetFences({immediate_fence});
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

    graphics_queue.submit2({submit_info}, immediate_fence);
    const auto result = device.waitForFences({immediate_fence}, true, std::numeric_limits<uint64_t>::max());
    if (result != vk::Result::eSuccess)
    {
        LOGGER_ERROR("Failed to wait for immediate command buffer submission: {}", vk::to_string(result));
        throw std::runtime_error("Failed to wait for immediate command buffer submission");
    }
}

} // portal
