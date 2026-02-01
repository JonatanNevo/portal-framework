//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <imgui.h>
#include "utils.h"

#include "imgui_fonts.h"

namespace portal
{
struct StringId;
}

namespace portal::imgui
{
#define IMGUI_DELETE_MOVE_COPY(Base)                             \
    Base(Base&&) = delete;                /* Move not allowed */ \
    Base &operator=(Base&&) = delete;     /* "" */               \
    Base(const Base&) = delete;           /* Copy not allowed */ \
    Base& operator=(const Base&) = delete /* "" */


struct ScopedWindow
{
    bool IsContentVisible;

    ScopedWindow(const char* name, bool* p_open = nullptr, const ImGuiWindowFlags flags = 0) { IsContentVisible = ImGui::Begin(name, p_open, flags); }
    ~ScopedWindow() { ImGui::End(); }

    explicit operator bool() const { return IsContentVisible; }

    IMGUI_DELETE_MOVE_COPY(ScopedWindow);
};


struct ScopedChild
{
    bool is_content_visible;

    ScopedChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags flags = 0)
    {
        is_content_visible = ImGui::BeginChild(str_id, size, child_flags, flags);
    }

    ScopedChild(ImGuiID id, const ImVec2& size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags flags = 0)
    {
        is_content_visible = ImGui::BeginChild(id, size, child_flags, flags);
    }

    ~ScopedChild() { ImGui::EndChild(); }

    explicit operator bool() const { return is_content_visible; }

    IMGUI_DELETE_MOVE_COPY(ScopedChild);
};

struct ScopedFont
{
    ScopedFont(const StringId& font_name) { ImGuiFonts::push_font(font_name); }
    ~ScopedFont() { ImGuiFonts::pop_font(); }

    IMGUI_DELETE_MOVE_COPY(ScopedFont);
};

struct ScopedColor
{
    ScopedColor(ImGuiCol idx, ImU32 col) { ImGui::PushStyleColor(idx, col); }
    ScopedColor(ImGuiCol idx, const ImVec4& col) { ImGui::PushStyleColor(idx, col); }
    ~ScopedColor() { ImGui::PopStyleColor(); }

    IMGUI_DELETE_MOVE_COPY(ScopedColor);
};

struct ScopedStyle
{
    template <typename T>
    ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }

    ~ScopedStyle() { ImGui::PopStyleVar(); }

    IMGUI_DELETE_MOVE_COPY(ScopedStyle);
};

struct ScopedItemWidth
{
    ScopedItemWidth(float item_width) { ImGui::PushItemWidth(item_width); }
    ~ScopedItemWidth() { ImGui::PopItemWidth(); }

    IMGUI_DELETE_MOVE_COPY(ScopedItemWidth);
};

struct ScopedTextWrapPos
{
    ScopedTextWrapPos(float wrap_pos_x = 0.0f) { ImGui::PushTextWrapPos(wrap_pos_x); }
    ~ScopedTextWrapPos() { ImGui::PopTextWrapPos(); }

    IMGUI_DELETE_MOVE_COPY(ScopedTextWrapPos);
};

struct ScopedButtonRepeat
{
    ScopedButtonRepeat(bool repeat) { ImGui::PushButtonRepeat(repeat); }
    ~ScopedButtonRepeat() { ImGui::PopButtonRepeat(); }

    IMGUI_DELETE_MOVE_COPY(ScopedButtonRepeat);
};

struct ScopedGroup
{
    ScopedGroup() { ImGui::BeginGroup(); }
    ~ScopedGroup() { ImGui::EndGroup(); }

    IMGUI_DELETE_MOVE_COPY(ScopedGroup);
};

struct ScopedID
{
    ScopedID(const char* str_id) { ImGui::PushID(str_id); }
    ScopedID(const char* str_id_begin, const char* str_id_end) { ImGui::PushID(str_id_begin, str_id_end); }
    ScopedID(const void* ptr_id) { ImGui::PushID(ptr_id); }
    ScopedID(int int_id) { ImGui::PushID(int_id); }
    ~ScopedID() { ImGui::PopID(); }

    IMGUI_DELETE_MOVE_COPY(ScopedID);
};

struct ScopedCombo
{
    bool is_open;

    ScopedCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0) { is_open = ImGui::BeginCombo(label, preview_value, flags); }
    ~ScopedCombo() { if (is_open) ImGui::EndCombo(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedCombo);
};

struct ScopedTreeNode
{
    bool is_open;

    ScopedTreeNode(const char* label) { is_open = ImGui::TreeNode(label); }

    ScopedTreeNode(const char* str_id, const char* fmt, ...) IM_FMTARGS(3)
    {
        va_list ap;
        va_start(ap, fmt);
        is_open = ImGui::TreeNodeV(str_id, fmt, ap);
        va_end(ap);
    }

    ScopedTreeNode(const void* ptr_id, const char* fmt, ...) IM_FMTARGS(3)
    {
        va_list ap;
        va_start(ap, fmt);
        is_open = ImGui::TreeNodeV(ptr_id, fmt, ap);
        va_end(ap);
    }

    ~ScopedTreeNode() { if (is_open) ImGui::TreePop(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedTreeNode);
};

struct ScopedTreeNodeV
{
    bool is_open;

    ScopedTreeNodeV(const char* str_id, const char* fmt, va_list args) IM_FMTLIST(3) { is_open = ImGui::TreeNodeV(str_id, fmt, args); }
    ScopedTreeNodeV(const void* ptr_id, const char* fmt, va_list args) IM_FMTLIST(3) { is_open = ImGui::TreeNodeV(ptr_id, fmt, args); }
    ~ScopedTreeNodeV() { if (is_open) ImGui::TreePop(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedTreeNodeV);
};

struct ScopedTreeNodeEx
{
    bool is_open;

    ScopedTreeNodeEx(const char* label, ImGuiTreeNodeFlags flags = 0)
    {
        IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen));
        is_open = ImGui::TreeNodeEx(label, flags);
    }

    ScopedTreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) IM_FMTARGS(4)
    {
        va_list ap;
        va_start(ap, fmt);
        IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen));
        is_open = ImGui::TreeNodeExV(str_id, flags, fmt, ap);
        va_end(ap);
    }

    ScopedTreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) IM_FMTARGS(4)
    {
        va_list ap;
        va_start(ap, fmt);
        IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen));
        is_open = ImGui::TreeNodeExV(ptr_id, flags, fmt, ap);
        va_end(ap);
    }

    ~ScopedTreeNodeEx() { if (is_open) ImGui::TreePop(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedTreeNodeEx);
};

struct ScopedTreeNodeIcon
{
    bool is_open;

    ScopedTreeNodeIcon(std::string_view title, const char* icon, ImVec2 size)
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

    ~ScopedTreeNodeIcon() { if (is_open) ImGui::TreePop(); }

    IMGUI_DELETE_MOVE_COPY(ScopedTreeNodeIcon);

private:
    struct TreeNodeConsts
    {
        ImVec2 frame_padding = {6.f, 6.f};
        float cursor_shift_delimiter = 3.f;
        float cursor_shift_offset = 3.7f;
    };
};

struct ScopedTreeNodeExV
{
    bool is_open;

    ScopedTreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) IM_FMTLIST(4)
    {
        IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen));
        is_open = ImGui::TreeNodeExV(str_id, flags, fmt, args);
    }

    ScopedTreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) IM_FMTLIST(4)
    {
        IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen));
        is_open = ImGui::TreeNodeExV(ptr_id, flags, fmt, args);
    }

    ~ScopedTreeNodeExV() { if (is_open) ImGui::TreePop(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedTreeNodeExV);
};

struct ScopedMainMenuBar
{
    bool is_open;

    ScopedMainMenuBar() { is_open = ImGui::BeginMainMenuBar(); }
    ~ScopedMainMenuBar() { if (is_open) ImGui::EndMainMenuBar(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedMainMenuBar);
};

struct ScopedMenuBar
{
    bool is_open;

    ScopedMenuBar() { is_open = ImGui::BeginMenuBar(); }
    ~ScopedMenuBar() { if (is_open) ImGui::EndMenuBar(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedMenuBar);
};

struct ScopedRectangleMenuBar
{
    bool is_open = false;

    explicit ScopedRectangleMenuBar(const ImRect& rect)
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

    ~ScopedRectangleMenuBar()
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

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedRectangleMenuBar);
};

struct ScopedMenu
{
    bool is_open;

    ScopedMenu(const char* label, bool enabled = true) { is_open = ImGui::BeginMenu(label, enabled); }
    ~ScopedMenu() { if (is_open) ImGui::EndMenu(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedMenu);
};

struct ScopedTooltip
{
    ScopedTooltip() { ImGui::BeginTooltip(); }
    ~ScopedTooltip() { ImGui::EndTooltip(); }

    IMGUI_DELETE_MOVE_COPY(ScopedTooltip);
};

struct ScopedPopup
{
    bool is_open;

    ScopedPopup(const char* str_id, ImGuiWindowFlags flags = 0) { is_open = ImGui::BeginPopup(str_id, flags); }
    ~ScopedPopup() { if (is_open) ImGui::EndPopup(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedPopup);
};

struct ScopedPopupContextItem
{
    bool is_open;

    ScopedPopupContextItem(const char* str_id = NULL, int mouse_button = 1) { is_open = ImGui::BeginPopupContextItem(str_id, mouse_button); }
    ~ScopedPopupContextItem() { if (is_open) ImGui::EndPopup(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedPopupContextItem);
};

struct ScopedPopupContextWindow
{
    bool is_open;

    ScopedPopupContextWindow(const char* str_id = NULL, bool also_over_items = true)
    {
        is_open = ImGui::BeginPopupContextWindow(str_id, also_over_items);
    }

    ~ScopedPopupContextWindow() { if (is_open) ImGui::EndPopup(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedPopupContextWindow);
};

struct ScopedPopupContextVoid
{
    bool is_open;

    ScopedPopupContextVoid(const char* str_id = NULL, int mouse_button = 1) { is_open = ImGui::BeginPopupContextVoid(str_id, mouse_button); }
    ~ScopedPopupContextVoid() { if (is_open) ImGui::EndPopup(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedPopupContextVoid);
};

struct ScopedPopupModal
{
    bool is_open;

    ScopedPopupModal(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) { is_open = ImGui::BeginPopupModal(name, p_open, flags); }
    ~ScopedPopupModal() { if (is_open) ImGui::EndPopup(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedPopupModal);
};

struct ScopedDragDropSource
{
    bool is_open;

    ScopedDragDropSource(ImGuiDragDropFlags flags = 0) { is_open = ImGui::BeginDragDropSource(flags); }
    ~ScopedDragDropSource() { if (is_open) ImGui::EndDragDropSource(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedDragDropSource);
};

struct ScopedDragDropTarget
{
    bool is_open;

    ScopedDragDropTarget() { is_open = ImGui::BeginDragDropTarget(); }
    ~ScopedDragDropTarget() { if (is_open) ImGui::EndDragDropTarget(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedDragDropTarget);
};

struct ScopedClipRect
{
    ScopedClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect)
    {
        ImGui::PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
    }

    ~ScopedClipRect() { ImGui::PopClipRect(); }

    IMGUI_DELETE_MOVE_COPY(ScopedClipRect);
};

struct ScopedChildFrame
{
    bool is_open;

    ScopedChildFrame(ImGuiID id, const ImVec2& size, ImGuiWindowFlags flags = 0) { is_open = ImGui::BeginChildFrame(id, size, flags); }
    ~ScopedChildFrame() { ImGui::EndChildFrame(); }

    explicit operator bool() const { return is_open; }

    IMGUI_DELETE_MOVE_COPY(ScopedChildFrame);
};

#undef IMGUI_DELETE_MOVE_COPY
}
