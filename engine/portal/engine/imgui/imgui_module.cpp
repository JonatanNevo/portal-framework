//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "imgui_module.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include "portal/engine/imgui/backends/imgui_impl_vulkan.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/renderer_context.h"
#include "../renderer/vulkan/render_target/vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_utils.h"
#include "portal/engine/window/glfw_window.h"

namespace portal
{
ImGuiModule::ImGuiModule(ModuleStack& stack, const Window& window, const renderer::vulkan::VulkanSwapchain& swapchain) : TaggedModule(stack, STRING_ID("ImGUI Module"))
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

    auto& renderer = get_dependency<Renderer>();

    //  create descriptor pool for IMGUI
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

    auto& vulkan_context = renderer.get_renderer_context().get_gpu_context();
    imgui_pool = (vulkan_context.get_device().get_handle()).createDescriptorPool(pool_info);

    const auto& vulkan_window = dynamic_cast<const GlfwWindow&>(window);
    ImGui_ImplGlfw_InitForVulkan(vulkan_window.get_handle(), true);

    auto color_formats = renderer.get_render_target().get_color_formats() ;
    const auto vulkan_color_formats = std::ranges::to<std::vector>(color_formats | std::views::transform([](const auto& format) { return renderer::vulkan::to_format(format); }));

    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = *vulkan_context.get_instance(),
        .PhysicalDevice = *vulkan_context.get_physical_device().get_handle(),
        .Device = *vulkan_context.get_device().get_handle(),
        .QueueFamily = static_cast<uint32_t>(vulkan_context.get_device().get_graphics_queue().get_family_index()),
        .Queue = vulkan_context.get_device().get_graphics_queue().get_handle(),
        .DescriptorPool = *imgui_pool,
        .MinImageCount = static_cast<uint32_t>(swapchain.get_image_count()),
        .ImageCount = static_cast<uint32_t>(swapchain.get_image_count()),
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfoKHR{
            .colorAttachmentCount = static_cast<uint32_t>(vulkan_color_formats.size()),
            .pColorAttachmentFormats = vulkan_color_formats.data()
        },
    };

    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();
}

ImGuiModule::~ImGuiModule()
{
    auto& vulkan_context = get_dependency<Renderer>().get_renderer_context().get_gpu_context();
    vulkan_context.get_device().wait_idle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    imgui_pool = nullptr;
}

void ImGuiModule::begin_frame(FrameContext&)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    //ImGuizmo::BeginFrame();
}

void ImGuiModule::end_frame(FrameContext& frame)
{
    PORTAL_PROF_ZONE();

    auto* rendering_context = std::any_cast<renderer::FrameRenderingContext>(&frame.rendering_context);

    const auto& renderer = get_dependency<Renderer>();
    auto& render_target = renderer.get_render_target();

    // set swapchain image layout to Attachment Optimal so we can draw it
    renderer::vulkan::transition_image_layout(
        rendering_context->command_buffer,
        rendering_context->image_context.draw_image,
        1,
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eNone,
        vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
    );

    ImGui::Render();

    const auto width = static_cast<uint32_t>(render_target.get_width());
    const auto height = static_cast<uint32_t>(render_target.get_height());

    vk::RenderingAttachmentInfo color_attachment = {
        .imageView = reference_cast<renderer::vulkan::VulkanImageView>(rendering_context->image_context.draw_image_view)->get_vk_image_view(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore
    };
    const vk::RenderingInfo rendering_info = {
        .renderArea = vk::Rect2D({0, 0}, {width, height}),
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr
    };

    // TODO: have imgui command buffers?
    rendering_context->command_buffer.beginRendering(rendering_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *rendering_context->command_buffer);
    rendering_context->command_buffer.endRendering();

    const ImGuiIO& io = ImGui::GetIO();
    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    // set draw image layout to Present so we can present it
    renderer::vulkan::transition_image_layout(
        rendering_context->command_buffer,
        rendering_context->image_context.draw_image,
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

void ImGuiModule::gui_update(FrameContext& frame)
{
    static std::array<float, 100> fps_history = {};
    static int fps_history_index = 0;

    fps_history[fps_history_index] = 1000.f / frame.stats.frame_time;
    fps_history_index = (fps_history_index + 1) % fps_history.size();

    ImGui::Begin("Stats");
    ImGui::Text("FPS %f", std::ranges::fold_left(fps_history, 0.f, std::plus<float>()) / 100.f);
    ImGui::Text("frametime %f ms", frame.stats.frame_time);
    ImGui::Text("draw time %f ms", frame.stats.mesh_draw_time);
    ImGui::Text("update time %f ms", frame.stats.scene_update_time);
    ImGui::Text("triangles %i", frame.stats.triangle_count);
    ImGui::Text("draws %i", frame.stats.drawcall_count);
    ImGui::End();
}
} // portal
