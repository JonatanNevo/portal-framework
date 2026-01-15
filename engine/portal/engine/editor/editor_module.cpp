//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "editor_module.h"

#include <imgui.h>

#include "portal/engine/imgui/backends/imgui_impl_vulkan.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/render_target/vulkan_render_target.h"

namespace portal
{
EditorModule::EditorModule(
    ModuleStack& stack,
    renderer::vulkan::VulkanContext& context,
    renderer::vulkan::VulkanSwapchain& swapchain,
    const Window& window
)
    : TaggedModule(stack, STRING_ID("Editor Module")),
      swapchain(swapchain),
      context(context),
      viewport_renderer(context, get_dependency<ResourcesModule>().get_registry()),
      im_gui_renderer(window, swapchain)
{
    renderer::RenderTargetProperties props{
        .width = swapchain.get_width(),
        // TODO: fetch size from some config
        .height = swapchain.get_height(),
        .attachments = {
            // TODO: Is this static? would this change based on settings? Do I need to recreate the render target on swapchain reset?
            .attachment_images = {
                // Present Image
                {
                    .format = renderer::vulkan::to_format(swapchain.get_color_format()),
                    .blend = false
                },
                // TODO: who is supposed to hold the depth image?
                // Depth Image
                {
                    .format = renderer::ImageFormat::Depth_32Float,
                    .blend = true,
                    .blend_mode = renderer::BlendMode::Additive
                }
            },
            .blend = true,
        },
        .transfer = true,
        .name = STRING_ID("viewport-render-target"),
    };
    viewport_render_target = make_reference<renderer::vulkan::VulkanRenderTarget>(props, context);

    const auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(viewport_render_target->get_image(0));
    vulkan_image->update_descriptor();
    auto& info = vulkan_image->get_image_info();
    viewport_descriptor_set = ImGui_ImplVulkan_AddTexture(
        info.sampler->get_vk_sampler(),
        info.view->get_vk_image_view(),
        static_cast<VkImageLayout>(vulkan_image->get_descriptor_image_info().imageLayout)
    );
}

EditorModule::~EditorModule()
{
    viewport_renderer.cleanup();
    ImGui_ImplVulkan_RemoveTexture(viewport_descriptor_set);
}

void EditorModule::begin_frame(FrameContext& frame)
{
    frame.rendering_context = swapchain.prepare_frame(frame);
    frame.active_scene = get_dependency<SystemOrchestrator>().get_active_scene();

    auto render_target = swapchain.get_current_render_target();
    im_gui_renderer.begin_frame(frame, render_target);
}

void EditorModule::gui_update(FrameContext& frame)
{
    gui_system.execute(*frame.ecs_registry);
    im_gui_renderer.gui_update(frame);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    if (ImGui::Begin("Viewport"))
    {
        auto window_size = ImGui::GetContentRegionAvail();
        auto recreated = viewport_render_target->resize(static_cast<size_t>(window_size.x), static_cast<size_t>(window_size.y), false);

        if (recreated)
        {
            frame.active_scene->set_viewport_bounds({0, 0, static_cast<uint32_t>(window_size.x), static_cast<uint32_t>(window_size.y)});
            const auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(viewport_render_target->get_image(0));
            auto& info = vulkan_image->get_image_info();
            vulkan_image->update_descriptor();

            ImGui_ImplVulkan_RemoveTexture(viewport_descriptor_set);
            viewport_descriptor_set = ImGui_ImplVulkan_AddTexture(
                info.sampler->get_vk_sampler(),
                info.view->get_vk_image_view(),
                static_cast<VkImageLayout>(vulkan_image->get_descriptor_image_info().imageLayout)
            );
        }
        ImGui::Image(
            reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(viewport_descriptor_set)),
            window_size,
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImVec4(1, 1, 1, 1),
            ImVec4(0, 0, 0, 0)
        );
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorModule::post_update(FrameContext& frame)
{
    viewport_renderer.begin_frame(frame, viewport_render_target);
    viewport_renderer.post_update(frame);
}

void EditorModule::end_frame(FrameContext& frame)
{
    viewport_renderer.end_frame(frame);
    im_gui_renderer.end_frame(frame);

    swapchain.present(frame);
}

void EditorModule::on_event(Event&)
{
}
} // portal
