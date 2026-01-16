//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "imgui_renderer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <ImGuizmo.h>

#include "imgui_fonts.h"
#include "portal/third_party/imgui/backends/imgui_impl_vulkan.h"
#include "portal/core/debug/profile.h"
#include "../renderer/vulkan/render_target/vulkan_render_target.h"
#include "portal/application/settings.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_utils.h"
#include "portal/engine/window/glfw_window.h"

namespace portal
{
ImGuiRenderer::ImGuiRenderer(ResourceRegistry& resource_registry, const Window& window, const renderer::vulkan::VulkanSwapchain& swapchain)
    : swapchain(swapchain)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Configure Fonts
    {
        ImGuiFontConfiguration roboto_bold{
            .name = STRING_ID("Bold"),
            .size = 18.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/Roboto/Roboto-Bold")),
        };
        ImGuiFonts::add(roboto_bold);

        ImGuiFontConfiguration roboto_large{
            .name = STRING_ID("Large"),
            .size = 36.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/Roboto/Roboto-Regular")),
        };
        ImGuiFonts::add(roboto_large);

        ImGuiFontConfiguration roboto_default{
            .name = STRING_ID("Default"),
            .size = 15.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/Roboto/Roboto-Regular")),
        };
        ImGuiFonts::add(roboto_default, true);

        ImGuiFontConfiguration fa{
            .name = STRING_ID("FontAwesome"),
            .size = 16.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/FontAwesome/fa6-solid")),
        };
        ImGuiFonts::add(fa, false, true);

        ImGuiFontConfiguration roboto_medium{
            .name = STRING_ID("Medium"),
            .size = 18.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/Roboto/Roboto-SemiMedium")),
        };
        ImGuiFonts::add(roboto_medium);

        ImGuiFontConfiguration roboto_small{
            .name = STRING_ID("Small"),
            .size = 12.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/Roboto/Roboto-SemiMedium")),
        };
        ImGuiFonts::add(roboto_small);

        ImGuiFontConfiguration roboto_extra_small{
            .name = STRING_ID("ExtraSmall"),
            .size = 10.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/Roboto/Roboto-SemiMedium")),
        };
        ImGuiFonts::add(roboto_extra_small);

        ImGuiFontConfiguration roboto_bold_title{
            .name = STRING_ID("BoldTitle"),
            .size = 16.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/Roboto/Roboto-Bold")),
        };
        ImGuiFonts::add(roboto_bold_title);

        ImGuiFontConfiguration roboto_bold_large{
            .name = STRING_ID("BoldLarge"),
            .size = 36.f,
            .font = resource_registry.immediate_load<Font>(STRING_ID("engine/fonts/Roboto/Roboto-Bold")),
        };
        ImGuiFonts::add(roboto_bold_large);
    }

    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

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
        .maxSets = 1000 * IM_ARRAYSIZE(pool_sizes),
        .poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes)),
        .pPoolSizes = pool_sizes
    };

    auto& vulkan_context = swapchain.get_context();
    imgui_pool = (vulkan_context.get_device().get_handle()).createDescriptorPool(pool_info);

    const auto& vulkan_window = dynamic_cast<const GlfwWindow&>(window);
    ImGui_ImplGlfw_InitForVulkan(vulkan_window.get_handle(), true);

    const auto color_format = swapchain.get_color_format();

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
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &color_format
        },
    };

    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();
}

ImGuiRenderer::~ImGuiRenderer()
{
    auto& vulkan_context = swapchain.get_context();
    vulkan_context.get_device().wait_idle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    imgui_pool = nullptr;
}

void ImGuiRenderer::begin_frame(const FrameContext&, const Reference<renderer::RenderTarget>& render_target)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    ImGuizmo::BeginFrame();

    current_render_target = render_target;
}

void ImGuiRenderer::end_frame(FrameContext& frame)
{
    PORTAL_PROF_ZONE();

    auto* rendering_context = std::any_cast<renderer::FrameRenderingContext>(&frame.rendering_context);

    // set swapchain image layout to Attachment Optimal so we can draw it
    renderer::vulkan::transition_image_layout(
        rendering_context->global_command_buffer,
        current_render_target->get_image(0),
        1,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eNone,
        vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
    );

    ImGui::Render();

    const auto width = static_cast<uint32_t>(current_render_target->get_width());
    const auto height = static_cast<uint32_t>(current_render_target->get_height());

    vk::RenderingAttachmentInfo color_attachment = {
        .imageView = reference_cast<renderer::vulkan::VulkanImageView>(current_render_target->get_image(0)->get_view())->get_vk_image_view(),
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
    rendering_context->global_command_buffer.beginRendering(rendering_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), rendering_context->global_command_buffer);
    rendering_context->global_command_buffer.endRendering();

    const ImGuiIO& io = ImGui::GetIO();
    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    // set draw image layout to Present so we can present it
    renderer::vulkan::transition_image_layout(
        rendering_context->global_command_buffer,
        current_render_target->get_image(0),
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
} // portal
