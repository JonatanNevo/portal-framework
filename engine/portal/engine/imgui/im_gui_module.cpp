//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "im_gui_module.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "portal/core/debug/profile.h"
#include "portal/engine/engine_context.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/renderer/vulkan/vulkan_utils.h"
#include "portal/engine/renderer/vulkan/vulkan_window.h"

namespace portal
{

ImGuiModule::ImGuiModule(const std::shared_ptr<EngineContext>& context): context(context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
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

    auto* renderer = context->renderer;

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

    auto& vulkan_context = renderer->get_renderer_context().get_gpu_context();
    imgui_pool = (vulkan_context.get_device().get_handle()).createDescriptorPool(pool_info);

    const auto vulkan_window = dynamic_cast<renderer::vulkan::VulkanWindow*>(context->window);
    ImGui_ImplGlfw_InitForVulkan(vulkan_window->window, true);

    const auto swapchain_format = vulkan_window->get_swapchain().get_color_format();

    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = *vulkan_context.get_instance(),
        .PhysicalDevice = *vulkan_context.get_physical_device().get_handle(),
        .Device = *vulkan_context.get_device().get_handle(),
        .Queue = vulkan_context.get_device().get_graphics_queue(),
        .DescriptorPool = *imgui_pool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfoKHR{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchain_format
        },
    };

    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();
}

ImGuiModule::~ImGuiModule()
{

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiModule::begin()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    //ImGuizmo::BeginFrame();
}

void ImGuiModule::end()
{
    PORTAL_PROF_ZONE();

    const auto vulkan_window = dynamic_cast<renderer::vulkan::VulkanWindow*>(context->window);
    auto& swapchain = vulkan_window->get_swapchain();

    const auto& command_buffer = swapchain.get_current_draw_command_buffer();

    // set swapchain image layout to Attachment Optimal so we can draw it
    vulkan::transition_image_layout(
        command_buffer,
        swapchain.get_current_draw_image(),
        1,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eTransferWrite,
        vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
        );


    ImGui::Render();

    const auto width = static_cast<uint32_t>(swapchain.get_width());
    const auto height = static_cast<uint32_t>(swapchain.get_height());

    vk::RenderingAttachmentInfo color_attachment = {
        .imageView = swapchain.get_current_draw_image_view(),
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
    command_buffer.beginRendering(rendering_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *command_buffer);
    command_buffer.endRendering();

    const ImGuiIO& io = ImGui::GetIO();
    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    // set swapchain image layout to Present so we can draw it
    vulkan::transition_image_layout(
        command_buffer,
        swapchain.get_current_draw_image(),
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

void ImGuiModule::on_gui_render() {
}
} // portal
