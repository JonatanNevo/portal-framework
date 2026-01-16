//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "viewport.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "selection_manager.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/imgui/utils.h"
#include "portal/third_party/imgui/backends/imgui_impl_vulkan.h"
#include "portal/engine/renderer/vulkan/render_target/vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"
#include "portal/third_party/font_awsome/IconsFontAwesome6.h"


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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(640.f, 360.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    if (ImGui::Begin("Viewport"))
    {
        is_mouse_over = ImGui::IsWindowHovered();
        is_focused = ImGui::IsWindowFocused();


        auto content_available = ImGui::GetContentRegionAvail();
        viewport_size = {content_available.x, content_available.y};

        if (viewport_size.x > 1 && viewport_size.y > 1)
        {
            const auto recreated = viewport_render_target->resize(viewport_size.x, viewport_size.y, false);

            if (recreated)
            {
                frame.active_scene->set_viewport_bounds({0, 0, viewport_size.x, viewport_size.y});
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
                content_available,
                ImVec2(0, 0),
                ImVec2(1, 1),
                ImVec4(1, 1, 1, 1),
                ImVec4(0, 0, 0, 0)
            );

            draw_gizmos_toolbar();
            draw_central_toolbar();

            if (is_focused && show_gizmos)
                draw_gizmos(frame);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

void Viewport::render(FrameContext& frame) const
{
    runtime_module.inner_post_update(frame, viewport_render_target);
    runtime_module.inner_end_frame(frame, false);
}

void Viewport::draw_gizmos_toolbar()
{
    imgui::ScopedStyle disable_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    imgui::ScopedStyle disable_window_border(ImGuiStyleVar_WindowBorderSize, 0.f);
    imgui::ScopedStyle window_rounding(ImGuiStyleVar_WindowRounding, 4.f);
    imgui::ScopedStyle disable_padding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    const float cursor_y_offset = ImGui::GetCursorStartPos().y + 10.f;
    constexpr float desired_height = 26.f;
    constexpr float button_size = 18.f;
    constexpr float edge_offset = 4.f;
    constexpr float number_of_buttons = 4.f;
    constexpr float gizmo_panel_background_width = edge_offset * 6.f + button_size * number_of_buttons + edge_offset * (number_of_buttons - 1.f) *
        2.f;

    ImGui::SetCursorPos(ImVec2(15.f, cursor_y_offset));
    {
        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2(gizmo_panel_background_width, desired_height);
        ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, IM_COL32(15, 15, 15, 127), 4.0f);

        constexpr float icon_font_size = 16.f;
        constexpr float padding = (button_size - icon_font_size) / 2.f;
        constexpr float actual_button_height = icon_font_size + padding * 2.f;

        auto vertical_name = "##gizmosV_Viewport"; // TODO: use viewport name?
        ImGui::BeginVertical(vertical_name, {gizmo_panel_background_width, desired_height});
        ImGui::Spring();

        auto horizontal_name = "##gizmosH_Viewport";
        ImGui::BeginHorizontal(horizontal_name, {gizmo_panel_background_width, actual_button_height});
        ImGui::Spring();
        {
            imgui::ScopedStyle enable_spacing(ImGuiStyleVar_ItemSpacing, ImVec2{edge_offset * 2.f, 0});
            imgui::ScopedStyle frame_padding(ImGuiStyleVar_FramePadding, ImVec2{padding, padding});
            imgui::ScopedColor button_bg(ImGuiCol_Button, ImVec4{0, 0, 0, 0});
            imgui::ScopedColor button_hover(ImGuiCol_ButtonHovered, ImVec4{1, 1, 1, 0.1f});
            imgui::ScopedColor button_active(ImGuiCol_ButtonActive, ImVec4{1, 1, 1, 0.2f});

            auto gizmo_button = [](auto icon)
            {
                return ImGui::Button(icon);
            };

            if (gizmo_button(ICON_FA_ARROW_POINTER))
                gizmo_type = -1;
            imgui::set_tooltip("Select");

            if (gizmo_button(ICON_FA_UP_DOWN_LEFT_RIGHT))
                gizmo_type = ImGuizmo::OPERATION::TRANSLATE;
            imgui::set_tooltip("Translate");

            if (gizmo_button(ICON_FA_ROTATE))
                gizmo_type = ImGuizmo::OPERATION::ROTATE;
            imgui::set_tooltip("Rotate");

            if (gizmo_button(ICON_FA_MAXIMIZE))
                gizmo_type = ImGuizmo::OPERATION::SCALE;
            imgui::set_tooltip("Scale");
        }
        ImGui::Spring();
        ImGui::EndHorizontal();
        ImGui::Spring();
        ImGui::EndVertical();
    }

    // Gizmo orientation window
    constexpr float offset_from_left = 10.f;
    constexpr float world_local_background_width = edge_offset * 6.0f + button_size + edge_offset * 2.0f;

    ImGui::SetCursorPos(ImVec2(15.f + gizmo_panel_background_width + offset_from_left, cursor_y_offset));

    {
        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2(world_local_background_width, desired_height);
        ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, IM_COL32(15, 15, 15, 127), 4.0f);

        auto world_local_horizontal_name = "##world_localH_Viewport";

        constexpr float wl_icon_font_size = 16.f;
        constexpr float wl_padding = (button_size - wl_icon_font_size) / 2.f;
        constexpr float wl_actual_button_height = wl_icon_font_size + wl_padding * 2.f;

        ImGui::BeginVertical("##world_localV_Viewport", {world_local_background_width, desired_height});
        ImGui::Spring();
        ImGui::BeginHorizontal(world_local_horizontal_name, {world_local_background_width, wl_actual_button_height});
        ImGui::Spring();
        {
            imgui::ScopedStyle frame_padding(ImGuiStyleVar_FramePadding, ImVec2{wl_padding, wl_padding});
            imgui::ScopedColor button_bg(ImGuiCol_Button, ImVec4{0, 0, 0, 0});
            imgui::ScopedColor button_hover(ImGuiCol_ButtonHovered, ImVec4{1, 1, 1, 0.1f});
            imgui::ScopedColor button_active(ImGuiCol_ButtonActive, ImVec4{1, 1, 1, 0.2f});

            if (ImGui::Button(gizmo_world_orientation ? ICON_FA_GLOBE : ICON_FA_CUBE))
                gizmo_world_orientation = !gizmo_world_orientation;
            imgui::set_tooltip("Toggles the transform gizmo between world and local space");
        }
        ImGui::Spring();
        ImGui::EndHorizontal();
        ImGui::Spring();
        ImGui::EndVertical();
    }
}

void Viewport::draw_central_toolbar()
{
    imgui::ScopedStyle disable_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    imgui::ScopedStyle disable_window_border(ImGuiStyleVar_WindowBorderSize, 0.f);
    imgui::ScopedStyle window_rounding(ImGuiStyleVar_WindowRounding, 4.f);
    imgui::ScopedStyle disable_padding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    const float cursor_y_offset = ImGui::GetCursorStartPos().y + 10.f;
    constexpr float button_size = 18.f + 5.f;
    constexpr float edge_offset = 4.f;
    constexpr float number_of_buttons = 3.f;
    constexpr float desired_height = 26.f + 5.f;
    constexpr float background_width = edge_offset * 6.f + button_size * number_of_buttons + edge_offset * (number_of_buttons - 1.f) * 2.f;

    ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x / 2.f - background_width / 2.f, cursor_y_offset));

    ImVec2 p_min = ImGui::GetCursorScreenPos();
    ImVec2 size = ImVec2(background_width, desired_height);
    ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

    ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, IM_COL32(15, 15, 15, 127), 4.0f);

    constexpr float icon_font_size = 16.f;
    constexpr float padding = (button_size - icon_font_size) / 2.f;
    constexpr float actual_button_height = icon_font_size + padding * 2.f;

    auto vertical_name = "##viewport_central_toolbarV_Viewport";
    ImGui::BeginVertical(vertical_name, {background_width, desired_height});
    ImGui::Spring();

    auto horizontal_name = "##viewport_central_toolbarH_Viewport";
    ImGui::BeginHorizontal(horizontal_name, {background_width, actual_button_height});
    ImGui::Spring();
    {
        imgui::ScopedStyle enableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(edge_offset * 2.0f, 0));
        imgui::ScopedStyle frame_padding(ImGuiStyleVar_FramePadding, ImVec2{padding, padding});
        imgui::ScopedColor button_bg(ImGuiCol_Button, ImVec4{0, 0, 0, 0});
        imgui::ScopedColor button_hover(ImGuiCol_ButtonHovered, ImVec4{1, 1, 1, 0.1f});
        imgui::ScopedColor button_active(ImGuiCol_ButtonActive, ImVec4{1, 1, 1, 0.2f});

        // TODO: Implement this
        ImGui::BeginDisabled();

        ImGui::Button(ICON_FA_PLAY);
        imgui::set_tooltip("Play (disabled)");
        ImGui::Button(ICON_FA_GEARS);
        imgui::set_tooltip("Simulate Physics (disabled)");
        ImGui::Button(ICON_FA_PAUSE);
        imgui::set_tooltip("Pause (disabled)");

        ImGui::EndDisabled();
    }

    ImGui::Spring();
    ImGui::EndHorizontal();
    ImGui::Spring();
    ImGui::EndVertical();
}

void Viewport::draw_gizmos(const FrameContext& frame)
{
    auto* scene = frame.active_scene;

    if (gizmo_type == -1)
        return;

    if (!SelectionSystem::has_selection(scene->get_scene_entity()))
        return;

    auto selected_entity = SelectionSystem::get_selected_entity(scene->get_scene_entity());

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    const ImVec2 window_pos = ImGui::GetWindowPos();
    ImGuizmo::SetRect(window_pos.x, window_pos.y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

    // TODO: Use the input system, the editor should block inputs from reaching the gameplay systems unless playing
    const bool snap = ImGui::IsKeyPressed(ImGuiKey_LeftCtrl);
    float snap_value = get_snap_value();
    float snap_values[3] = {snap_value, snap_value, snap_value};

    auto main_camera = scene->get_main_camera_entity();
    auto camera = main_camera.get_component<CameraComponent>();

    auto projection_matrix = camera.projection;
    auto view_matrix = camera.view;

    auto& entity_transform = selected_entity.get_component<TransformComponent>();

    auto transform = entity_transform.get_world_matrix();
    if (ImGuizmo::Manipulate(
        glm::value_ptr(view_matrix),
        glm::value_ptr(projection_matrix),
        static_cast<ImGuizmo::OPERATION>(gizmo_type),
        gizmo_world_orientation ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL,
        glm::value_ptr(transform),
        nullptr,
        snap ? snap_values : nullptr
    ))
    {
        auto parent = selected_entity.get_parent();
        if (parent)
        {
            auto parent_transform = parent.get_component<TransformComponent>().get_world_matrix();
            transform = glm::inverse(parent_transform) * transform;
        }

        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(transform, scale, rotation, translation, skew, perspective);


        selected_entity.patch_component<TransformComponent>(
            [&](TransformComponent& comp)
            {
                switch (gizmo_type)
                {
                case ImGuizmo::TRANSLATE:
                    {
                        comp.set_translation(translation);
                        break;
                    }
                case ImGuizmo::ROTATE:
                    {
                        comp.set_rotation(rotation);
                        break;
                    }
                case ImGuizmo::SCALE:
                    {
                        comp.set_scale(scale);
                        break;
                    }
                default:
                    break;
                }
            }
        );
    }
}

float Viewport::get_snap_value() const
{
    switch (gizmo_type)
    {
    case ImGuizmo::OPERATION::TRANSLATE: return translation_snap_value;
    case ImGuizmo::OPERATION::ROTATE: return rotation_snap_value;
    case ImGuizmo::OPERATION::SCALE: return scale_snap_value;
    default:
        break;
    }
    return 0.f;
}
} // portal
