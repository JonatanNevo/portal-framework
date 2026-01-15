//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "viewport.h"

#include <imgui.h>

#include "portal/engine/imgui/backends/imgui_impl_vulkan.h"
#include "portal/engine/renderer/vulkan/render_target/vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"


namespace portal
{
Viewport::Viewport(const renderer::vulkan::VulkanSwapchain& swapchain, RuntimeModule& runtime_module) : runtime_module(runtime_module)
{
    renderer::RenderTargetProperties props{
        .width = swapchain.get_width(),
        // TODO: fetch size from some config
        .height = swapchain.get_height(),
        .attachments = renderer::AttachmentProperties{
            // TODO: Is this static? would this change based on settings? Do I need to recreate the render target on swapchain reset?
            .attachment_images = std::vector<renderer::AttachmentTextureProperty>{
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
    viewport_render_target = make_reference<renderer::vulkan::VulkanRenderTarget>(props, swapchain.get_context());

    const auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(viewport_render_target->get_image(0));
    vulkan_image->update_descriptor();
    const auto& info = vulkan_image->get_image_info();
    viewport_descriptor_set = ImGui_ImplVulkan_AddTexture(
        info.sampler->get_vk_sampler(),
        info.view->get_vk_image_view(),
        static_cast<VkImageLayout>(vulkan_image->get_descriptor_image_info().imageLayout)
    );
}

Viewport::~Viewport()
{
    ImGui_ImplVulkan_RemoveTexture(viewport_descriptor_set);
}

void Viewport::on_gui_update(const FrameContext& frame)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    if (ImGui::Begin("Viewport"))
    {
        const auto window_size = ImGui::GetContentRegionAvail();
        const auto recreated = viewport_render_target->resize(static_cast<size_t>(window_size.x), static_cast<size_t>(window_size.y), false);

        if (recreated)
        {
            frame.active_scene->set_viewport_bounds({0, 0, static_cast<uint32_t>(window_size.x), static_cast<uint32_t>(window_size.y)});
            const auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(viewport_render_target->get_image(0));
            const auto& info = vulkan_image->get_image_info();
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

void Viewport::render(FrameContext& frame) const
{
    runtime_module.inner_post_update(frame, viewport_render_target);
    runtime_module.inner_end_frame(frame, false);
}

} // portal
