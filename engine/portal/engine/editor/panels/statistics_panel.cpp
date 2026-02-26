//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "statistics_panel.h"

#include <algorithm>
#include <array>
#include <imgui.h>

#include "portal/application/frame_context.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"

namespace portal
{
void StatisticsPanel::on_gui_render(EditorContext& editor_context, FrameContext& frame_context, bool& is_open)
{
    static constexpr int history_size = 256;

    static std::array<float, history_size> fps_history = {};
    static std::array<float, history_size> frame_time_history_ms = {};
    static std::array<float, history_size> draw_time_history_ms = {};
    static std::array<float, history_size> update_time_history_ms = {};
    static int history_index = 0;

    fps_history[history_index] = 1000.f / frame_context.stats.frame_time;
    frame_time_history_ms[history_index] = frame_context.stats.frame_time;
    draw_time_history_ms[history_index] = frame_context.stats.mesh_draw_time;
    update_time_history_ms[history_index] = frame_context.stats.scene_update_time;

    history_index = (history_index + 1) % history_size;

    const auto avg = [](const auto& arr) -> float
    {
        return std::ranges::fold_left(arr, 0.f, std::plus<float>()) / static_cast<float>(arr.size());
    };

    if (ImGui::Begin("Statistics", &is_open))
    {
        auto window_background = editor_context.theme.scoped_color(ImGuiCol_WindowBg, imgui::ThemeColors::Background3);
        auto child_background = editor_context.theme.scoped_color(ImGuiCol_ChildBg, imgui::ThemeColors::Background3);

        if (ImGui::BeginTabBar("##statistics_tabs"))
        {
            if (ImGui::BeginTabItem("Renderer"))
            {
                ImGui::TextUnformatted("Type: Vulkan");
                const auto& [vendor, device, version] = editor_context.vulkan_context.get_capabilities();
                ImGui::Text("Vendor: %s", vendor.c_str());
                ImGui::Text("Device: %s", device.c_str());
                ImGui::Text("Version: %d.%d.%d", version.major, version.minor, version.patch);

                ImGui::Separator();
                ImGui::Text("Frame Time: %.3f ms", avg(frame_time_history_ms));
                ImGui::Text("FPS %.2f", avg(fps_history));

                ImGui::Separator();
                {
                    imgui::ScopedFont bold(STRING_ID("Bold"));
                    ImGui::Text("Frame Data");
                }

                ImGui::Text("Draw Time: %.3f ms", avg(draw_time_history_ms));
                ImGui::Text("Update Time %.3f ms", avg(update_time_history_ms));

                ImGui::Separator();
                ImGui::Text("Triangles %i", frame_context.stats.triangle_count);
                ImGui::Text("Draws %i", frame_context.stats.drawcall_count);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Memory"))
            {
                ImGui::Text("Not Implemented");
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();
}
} // portal
