//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "imgui_scoped.h"

#include "utils.h"

namespace portal::imgui
{
ScopedTreeNodeIcon::ScopedTreeNodeIcon(std::string_view title, const char* icon, ImVec2 size)
{
    constexpr ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_Framed
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_AllowOverlap
        | ImGuiTreeNodeFlags_FramePadding
        | ImGuiTreeNodeFlags_DefaultOpen;

    constexpr TreeNodeConsts consts{};

    ScopedStyle frame_rounding{ImGuiStyleVar_FrameRounding, 0.f};
    ScopedStyle frame_padding{ImGuiStyleVar_FramePadding, consts.frame_padding};
    ScopedID node_id{title.data()};

    is_open = ImGui::TreeNodeEx("##dummy_id", tree_node_flags);
    ImGui::SameLine();
    shift_cursor(0.f, size.y / consts.cursor_shift_delimiter - consts.cursor_shift_offset);
    ImGui::TextUnformatted(icon);
    ImGui::SameLine();
    shift_cursor(0.f, -(size.y / consts.cursor_shift_delimiter) + consts.cursor_shift_offset);
    ImGui::TextUnformatted(title.data());
}

ScopedRectangleMenuBar::ScopedRectangleMenuBar(const ImRect& rect)
{
    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGui::BeginGroup();
    ImGui::PushID("##menubar");

    const auto padding = window->WindowPadding;

    // We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
    // We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
    ImRect bar_rect = imgui::rect_offset(rect, 0.f, padding.y);
    ImRect clip_rect(
        IM_ROUND(ImMax(window->Pos.x, bar_rect.Min.x + window->WindowBorderSize + window->Pos.x - 10.0f)),
        IM_ROUND(bar_rect.Min.y + window->WindowBorderSize + window->Pos.y),
        IM_ROUND(ImMax(bar_rect.Min.x + window->Pos.x, bar_rect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize))),
        IM_ROUND(bar_rect.Max.y + window->Pos.y)
    );

    clip_rect.ClipWith(window->OuterRectClipped);
    ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

    // We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
    window->DC.CursorPos = window->DC.CursorMaxPos = ImVec2(bar_rect.Min.x + window->Pos.x, bar_rect.Min.y + window->Pos.y);
    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
    window->DC.MenuBarAppending = true;
    ImGui::AlignTextToFramePadding();
    is_open = true;
}

ScopedRectangleMenuBar::~ScopedRectangleMenuBar()
{
    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    auto& context = *GImGui;

    // When a move request within one of our child menu failed, capture the request to navigate among our siblings.
    if (ImGui::NavMoveRequestButNoResultYet() && (context.NavMoveDir == ImGuiDir_Left || context.NavMoveDir == ImGuiDir_Right) && (context.
        NavWindow->Flags &
        ImGuiWindowFlags_ChildMenu))
    {
        // Try to find out if the request is for one of our child menu
        const ImGuiWindow* nav_earliest_child = context.NavWindow;
        while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
            nav_earliest_child = nav_earliest_child->ParentWindow;
        if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && (context.
            NavMoveFlags
            & ImGuiNavMoveFlags_Forwarded) == 0)
        {
            // To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
            // This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth bothering)
            constexpr ImGuiNavLayer layer = ImGuiNavLayer_Menu;
            IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check
            ImGui::FocusWindow(window);
            ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
            context.NavCursorVisible = false; // Hide highlight for the current frame so we don't see the intermediary selection.
            context.NavHighlightItemUnderNav = context.NavMousePosDirty = true;
            ImGui::NavMoveRequestForward(context.NavMoveDir, context.NavMoveClipDir, context.NavMoveFlags, context.NavMoveScrollFlags); // Repeat
        }
    }

    IM_MSVC_WARNING_SUPPRESS(6011) // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
    // IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar); // NOTE(Yan): Needs to be commented out because Jay
    IM_ASSERT(window->DC.MenuBarAppending);
    ImGui::PopClipRect();
    ImGui::PopID();
    window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x;
    // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
    context.GroupStack.back().EmitItem = false;
    ImGui::EndGroup(); // Restore position on layer 0
    window->DC.LayoutType = ImGuiLayoutType_Vertical;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
    window->DC.MenuBarAppending = false;
}
}
