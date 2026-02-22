//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "content_browser_panel.h"

#include "items/resource_item.h"

#include "portal/engine/scene/scene.h"
#include "portal/core/files/file_system.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/editor/selection_system.h"
#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/engine/imgui/tree_node_with_icon.h"
#include "portal/engine/imgui/widgets/options_button.h"
#include "portal/engine/imgui/widgets/search_widget.h"
#include "portal/engine/project/project.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/resources/database/folder_resource_database.h"
#include "portal/serialization/archive/json_archive.h"

namespace portal
{
static auto logger = Log::get_logger("Content Browser");

void content_browser::SelectionStack::copy_from(SelectionStack& other)
{
    selections.assign(other.begin(), other.end());
}

void content_browser::SelectionStack::copy_from(const std::vector<StringId>& other)
{
    selections.assign(other.begin(), other.end());
}

void content_browser::SelectionStack::select(const StringId& id)
{
    if (is_selected(id))
        return;

    selections.push_back(id);
}

void content_browser::SelectionStack::deselect(const StringId& id)
{
    if (!is_selected(id))
        return;

    for (auto it = selections.begin(); it != selections.end(); ++it)
    {
        if (id == *it)
        {
            selections.erase(it);
            return;
        }
    }
}

bool content_browser::SelectionStack::is_selected(const StringId& id) const
{
    return std::ranges::find(selections, id) != selections.end();
}

void content_browser::SelectionStack::clear()
{
    selections.clear();
}

size_t content_browser::SelectionStack::selection_count() const
{
    return selections.size();
}

const StringId* content_browser::SelectionStack::selection_data() const
{
    return selections.data();
}

StringId content_browser::SelectionStack::operator[](const size_t index) const
{
    PORTAL_ASSERT(index < selections.size(), "Index out of bounds");
    return selections[index];
}

std::vector<StringId>::iterator content_browser::SelectionStack::begin()
{
    return selections.begin();
}

std::vector<StringId>::iterator content_browser::SelectionStack::end()
{
    return selections.end();
}

std::vector<StringId>::const_iterator content_browser::SelectionStack::begin() const
{
    return selections.begin();
}

std::vector<StringId>::const_iterator content_browser::SelectionStack::end() const
{
    return selections.end();
}

using namespace content_browser;

ContentBrowserPanel::ContentBrowserPanel(const EditorContext& editor_context) : project(editor_context.project)
{
    selection_context = editor_context.ecs_registry.create_entity(STRING_ID("Content Browser"));
    std::memset(search_buffer.data(), 0, search_buffer.size());

    refresh();
}

struct ContentBrowserConsts
{
    ImVec2 item_spacing = ImVec2(8.0f, 8.0f);
    ImVec2 frame_padding = ImVec2(4.0f, 4.0f);
    ImVec2 cell_padding = ImVec2(10.0f, 2.0f);
    float outliner_column_width = 300.f;

    float shadow_rect_y_offset = 10.f;

    float topbar_height = 26.f;
    float bottombar_height = 32.f;
    ImVec2 item_menu_spacing = ImVec2{4.f, 4.f};

    float padding_for_outline = 2.f;
    float scroll_bar_offset = 20.f;
    float scrollbar_padding = 2.f;
    float row_spacing = 12.f;
};

void ContentBrowserPanel::on_gui_render(EditorContext& editor_context, FrameContext&)
{
    static ContentBrowserConsts consts;
    imgui::draw_consts_controls("Content Browser Consts", consts);

    is_hovered = false;
    is_focused = false;

    bool open;

    imgui::ScopedWindow content_browser_window("Content Browser", &open, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    if (!content_browser_window)
        return;

    is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    is_focused = ImGui::IsWindowFocused(ImGuiHoveredFlags_RootAndChildWindows);

    imgui::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, consts.item_spacing);
    imgui::ScopedStyle padding(ImGuiStyleVar_FramePadding, consts.frame_padding);
    imgui::ScopedStyle cell_padding(ImGuiStyleVar_CellPadding, consts.frame_padding);

    constexpr auto table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV;

    imgui::push_id();

    if (ImGui::BeginTable("##ContentBrowserTable", 2, table_flags, ImVec2(0, 0)))
    {
        ImGui::TableSetupColumn("Hierarchy", 0, consts.outliner_column_width);
        ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        // Content Hierarchy
        ImGui::BeginChild("##DirectoryHierarchy", ImVec2(0, 0), true);
        {
            imgui::ScopedStyle disable_spacing(ImGuiStyleVar_ItemSpacing, ImVec2{0.f, 0.f});
            imgui::ScopedColor item_header(ImGuiCol_Header, IM_COL32_DISABLE);
            imgui::ScopedColor item_header_active(ImGuiCol_HeaderActive, IM_COL32_DISABLE);

            if (base_directory)
            {
                std::vector<Reference<DirectoryInfo>> subdirs;
                subdirs.reserve(base_directory->subdirectories.size());
                for (auto& dir : base_directory->subdirectories | std::views::values)
                    subdirs.push_back(dir);

                std::ranges::sort(subdirs, [](const auto& a, const auto& b) { return a->path.stem().string() < b->path.stem().string(); });
                for (const auto& dir : subdirs)
                    render_directory_tree(editor_context, dir);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{4.f, 4.f});
            if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("New Folder"))
                {
                    auto filepath = FileSystem::get_unique_file_name(project.get_resource_directory() / "New Folder");
                    const bool created = FileSystem::create_directory(filepath);

                    if (created)
                        refresh();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Show in Explorer"))
                    FileSystem::open_directory_in_explorer(project.get_resource_directory());

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);

        // Directory Content
        ImGui::BeginChild(
            "##DirectoryContent",
            ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetWindowHeight() - consts.topbar_height - consts.bottombar_height)
        );
        {
            {
                imgui::ScopedStyle disable_borders(ImGuiStyleVar_FrameBorderSize, 0.0f);
                draw_topbar(editor_context, consts.topbar_height);
            }

            ImGui::Separator();

            ImGui::BeginChild("Scrolling");
            {
                auto botton_color = editor_context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Primary1);
                auto botton_hovered_color = editor_context.theme.scoped_color(ImGuiCol_ButtonHovered, imgui::ThemeColors::Primary2);

                {
                    imgui::ScopedStyle item_menu_spacing(ImGuiStyleVar_ItemSpacing, consts.item_menu_spacing);
                    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
                    {
                        if (ImGui::BeginMenu("New"))
                        {
                            if (ImGui::MenuItem("Folder"))
                            {
                                const auto filepath = FileSystem::get_unique_file_name(current_directory->path / "New Folder");

                                const bool created = FileSystem::create_directory(filepath);
                                if (created)
                                {
                                    refresh();
                                    const auto& dir_info = get_directory(filepath);
                                    const size_t index = current_items.items.size();
                                    if (index != ItemList::invalid_item)
                                    {
                                        SelectionSystem::deselect_all(selection_context);
                                        SelectionSystem::select(dir_info->id, selection_context);
                                        current_items[index]->start_renaming();
                                    }
                                }
                            }

                            if (ImGui::MenuItem("Scene"))
                                create_resource<Scene>("new_scene.pscene", editor_context);

                            if (ImGui::MenuItem("Material"))
                                create_resource<renderer::Material>("new_material.pmaterial", editor_context);

                            ImGui::EndMenu();
                        }

                        if (ImGui::MenuItem("Import"))
                        {
                            auto filepath = FileSystem::open_file_dialog();
                            if (!filepath.empty())
                            {
                                LOGGER_WARN("Importing is not yet implemented, skipping...");
                                // FileSystem::copy(filepath, current_directory->path);
                                // TODO: Update resource database?
                                refresh();
                            }
                        }

                        if (ImGui::MenuItem("Refresh"))
                            refresh();

                        ImGui::Separator();

                        if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, SelectionSystem::selection_count(selection_context) > 0))
                            copied_resources.copy_from(SelectionSystem::get_selections(selection_context));


                        if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, copied_resources.selection_count() > 0))
                            paste_copied_resources();

                        if (ImGui::MenuItem("Duplicate", "Ctrl+D", nullptr, SelectionSystem::selection_count(selection_context) > 0))
                        {
                            copied_resources.copy_from(SelectionSystem::get_selections(selection_context));
                            paste_copied_resources();
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("Show in Explorer"))
                        {
                            FileSystem::open_directory_in_explorer(current_directory->path);
                        }

                        ImGui::EndPopup();
                    }
                }

                const float scroll_bar_offset = consts.scroll_bar_offset + ImGui::GetStyle().ScrollbarSize;
                const float paned_width = ImGui::GetContentRegionAvail().x - scroll_bar_offset;
                const float cell_size = 128.f /* thumbnail size */ + consts.scrollbar_padding + consts.padding_for_outline;

                int column_count = static_cast<int>(paned_width / cell_size);
                if (column_count < 1)
                    column_count = 1;

                {
                    imgui::ScopedStyle item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(consts.padding_for_outline, consts.row_spacing));
                    ImGui::Columns(column_count, nullptr, false);

                    imgui::ScopedStyle disable_border(ImGuiStyleVar_FrameBorderSize, 0.0f);
                    imgui::ScopedStyle disable_padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                    render_items(editor_context);
                }

                render_delete_dialog(editor_context);
                render_delete_folder_dialog(editor_context);
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        draw_bottombar(editor_context, consts.bottombar_height);

        ImGui::EndTable();
    }

    imgui::pop_id();
}

Reference<DirectoryInfo> ContentBrowserPanel::get_directory(const std::filesystem::path& path) const
{
    if (path.string() == "" || path.string() == ".")
        return current_directory;

    for (const auto& dir : directories | std::views::values)
    {
        if (dir->path == path)
            return dir;
    }

    return nullptr;
}

void ContentBrowserPanel::register_item_activation_callback(StringId, std::function<void(const ResourceReference<Resource>)>) {
}

void ContentBrowserPanel::register_resource_created_callback(std::function<void(const ResourceReference<Resource>)>) {
}

void ContentBrowserPanel::register_resource_deleted_callback(std::function<void(const ResourceReference<Resource>)>) {
}

bool ContentBrowserPanel::delete_directory(const DirectoryInfo& info)
{
    const bool deleted = FileSystem::remove(info.path);
    if (!deleted)
    {
        LOGGER_ERROR("Failed to delete directory: {}", info.path.string());
        return false;
    }

    for (const auto resource : info.resources)
    {
        // TODO: unload from resource manager
        project.get_resource_database().remove(resource);
    }

    if (directories.contains(info.id))
    {
        remove_directory_info(directories.at(info.id));
    }

    return true;
}

StringId ContentBrowserPanel::process_directory(const std::filesystem::path& directory_path, const Reference<DirectoryInfo>& parent)
{
    const auto directory = get_directory(directory_path);
    if (directory)
        return directory->id;

    Reference<DirectoryInfo> info = make_reference<DirectoryInfo>();
    info->id = STRING_ID(directory_path.generic_string());
    info->parent = parent;
    info->path = directory_path;

    for (auto entry : std::filesystem::directory_iterator(directory_path))
    {
        if (entry.is_directory())
        {
            StringId subdir = process_directory(entry.path(), info);
            info->subdirectories[subdir] = directories[subdir];
        }

        if (entry.path().extension() == FolderResourceDatabase::RESOURCE_METADATA_EXTENSION)
        {
            JsonArchive archiver;
            archiver.read(entry.path());

            // TODO: Add serialization checks
            auto resource_metadata = SourceMetadata::dearchive(archiver);
            auto resource_id = resource_metadata.resource_id;

            auto meta = project.get_resource_database().find(resource_id);
            if (meta.has_value())
            {
                info->resources.push_back(resource_id);
            }
        }
    }

    directories[info->id] = info;
    return info->id;
}

void ContentBrowserPanel::change_directory(const Reference<DirectoryInfo>& directory)
{
    if (!directory)
        return;

    update_navigation_path = true;
    current_items.clear();

    if (std::strlen(search_buffer.data()) == 0)
    {
        for (auto& info : directory->subdirectories | std::views::values)
        {
            current_items.items.push_back(make_reference<DirectoryItem>(info));
        }

        for (auto id : directory->resources)
        {
            auto meta = project.get_resource_database().find(id);
            if (!meta.has_value())
                continue;

            auto item_icon = EditorIcon::File;

            current_items.items.push_back(make_reference<ResourceItem>(meta.value(), item_icon));
        }
    }
    else
    {
        current_items = search(std::string(search_buffer.data()), directory);
    }

    sort_item_list();

    prev_directory = current_directory;
    current_directory = directory;

    clear_selection();
    // TODO: generate thumbnails
}

void ContentBrowserPanel::on_browse_back()
{
    next_directory = current_directory;
    prev_directory = current_directory->parent.lock();
    change_directory(prev_directory);
}

void ContentBrowserPanel::on_browse_forward()
{
    change_directory(next_directory);
}

namespace
{
    bool directory_node(
        const std::string& id,
        const std::string& label,
        const ImGuiTreeNodeFlags flags = 0,
        const vk::DescriptorSet icon = nullptr,
        const vk::DescriptorSet icon_opened = nullptr
    )
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        return tree_node_with_icon(icon, icon_opened, window->GetID(id.c_str()), flags, label.c_str(), nullptr);
    }
}

void ContentBrowserPanel::render_directory_tree(EditorContext& editor_context, Reference<DirectoryInfo> directory)
{
    auto name = directory->path.filename().string();
    auto id = fmt::format("{}_tree_node", name);
    bool prev_state = ImGui::TreeNodeUpdateNextOpen(ImGui::GetID(id.c_str()), 0);

    // ImGui item height hack
    auto* window = ImGui::GetCurrentWindow();
    window->DC.CurrLineSize.y = 20.f;
    window->DC.CurrLineTextBaseOffset = 3.f;
    //---------------------------------------------

    const ImRect item_rect = {
        window->WorkRect.Min.x,
        window->DC.CursorPos.y,
        window->WorkRect.Max.x,
        window->DC.CursorPos.y + window->DC.CurrLineSize.y
    };

    const bool is_item_clicked = [&item_rect]
    {
        if (ImGui::IsMouseHoveringRect(item_rect.Min, item_rect.Max))
        {
            return ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        }
        return false;
    }();

    const bool is_window_focused = ImGui::IsWindowFocused();

    auto fill_with_color = [&](const ImColor& color)
    {
        const auto bg_color = ImGui::ColorConvertFloat4ToU32(color);
        ImGui::GetWindowDrawList()->AddRectFilled(item_rect.Min, item_rect.Max, bg_color);
    };

    auto check_if_any_descendant_selected = [&](DirectoryInfo& info, auto self)
    {
        if (info.id == current_directory->id)
            return true;

        if (!info.subdirectories.empty())
        {
            for (auto& child : info.subdirectories | std::views::values)
            {
                if (self(*child, self))
                    return true;
            }
        }

        return false;
    };

    const bool is_any_descendant_selected = check_if_any_descendant_selected(*directory, check_if_any_descendant_selected);
    const bool is_active_directory = directory->id == current_directory->id;

    const auto flags = (is_active_directory ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanFullWidth;

    // Fill background
    //----------------
    if (is_active_directory || is_item_clicked)
    {
        if (is_window_focused)
        {
            fill_with_color(editor_context.theme[imgui::ThemeColors::Primary1]);
        }
        else
        {
            const auto col = imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Primary1], 0.8f);
            fill_with_color(imgui::color_with_multiplied_saturation(col, 0.7f));
        }

        ImGui::PushStyleColor(ImGuiCol_Text, editor_context.theme[imgui::ThemeColors::TextDarker]);
    }
    else if (is_any_descendant_selected)
    {
        fill_with_color(editor_context.theme[imgui::ThemeColors::Primary2]);
    }

    // Tree Node
    //----------

    bool open = directory_node(
        id,
        name,
        flags,
        editor_context.icons.get_descriptor(EditorIcon::Directory),
        editor_context.icons.get_descriptor(EditorIcon::DirectoryOpen)
    );

    if (is_active_directory || is_item_clicked)
        ImGui::PopStyleColor();

    imgui::shift_cursor(0.f, 3.f);

    // Create Menu
    //------------
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::BeginMenu("New"))
        {
            if (ImGui::MenuItem("Folder"))
            {
                const bool created = FileSystem::create_directory(directory->path / "New Folder");
                if (created)
                    refresh();
            }

            if (ImGui::MenuItem("Scene"))
                create_resource_in_directory<Scene>("new_scene.pscene", editor_context, directory);

            if (ImGui::MenuItem("Material"))
                create_resource_in_directory<renderer::Material>("new_material.pmaterial", editor_context, directory);

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Delete"))
        {
            open_delete_folder_popup = true;
            pending_removal_directory = directory;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Show In Explorer"))
        {
            FileSystem::open_directory_in_explorer(directory->path);
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // Draw children
    //--------------

    if (open)
    {
        std::vector<Reference<DirectoryInfo>> directories_to_render;
        directories_to_render.reserve(directory->subdirectories.size());
        for (auto& dir : directory->subdirectories | std::views::values)
        {
            directories_to_render.emplace_back(dir);
        }

        std::ranges::sort(directories_to_render, [](const auto& a, const auto& b) { return a->path.stem().string() < b->path.stem().string(); });

        for (const auto& child : directories_to_render)
            render_directory_tree(editor_context, child);
    }

    update_drop_area(directory);

    if (open != prev_state && !is_active_directory)
    {
        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f))
            change_directory(directory);
    }

    if (open)
        ImGui::TreePop();
}

struct TopbarConsts
{
    float edge_offset = 4.0f;
    ImVec2 item_spacing = {2.f, 2.f};
    float min_icon_size = 24.f;
    float icon_padding = 3.f;
    float search_shift = 4.f;
    float search_width = 200.f;
};


void ContentBrowserPanel::draw_topbar(EditorContext& editor_context, float height)
{
    constexpr static TopbarConsts consts;

    ImGui::BeginChild("##top_bar", ImVec2(0, height));
    ImGui::BeginHorizontal("##top_bar", ImGui::GetWindowSize());
    {
        // Navigation buttons
        {
            imgui::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, consts.item_spacing);

            auto content_browser_button = [&](const char* label, const EditorIcon icon)
            {
                const ImU32 color = ImGui::ColorConvertFloat4ToU32(editor_context.theme[imgui::ThemeColors::Background2]);
                const ImU32 color_pressed = imgui::color_with_multiplied_value(color, 0.8f);

                imgui::ScopedColor scoped_color_button(ImGuiCol_Button, color);
                imgui::ScopedColor scoped_color_button_hovered(ImGuiCol_ButtonHovered, color);
                imgui::ScopedColor scoped_color_button_active(ImGuiCol_ButtonActive, color_pressed);

                const float icon_size = std::min(consts.min_icon_size, height);
                const bool clicked = ImGui::Button(label, ImVec2(icon_size, icon_size));

                auto text_darker = editor_context.theme[imgui::ThemeColors::TextDarker];
                imgui::draw_button_image(
                    editor_context.icons.get_descriptor(icon),
                    text_darker,
                    imgui::color_with_multiplied_value(text_darker, 1.2f),
                    imgui::color_with_multiplied_value(text_darker, 0.8f),
                    imgui::expand_rect(imgui::get_item_rect(), -consts.icon_padding, -consts.icon_padding)
                );

                return clicked;
            };

            if (content_browser_button("##back", EditorIcon::Back))
            {
                on_browse_back();
            }
            imgui::set_tooltip("Previous directory");

            if (content_browser_button("##forward", EditorIcon::Forward))
            {
                on_browse_forward();
            }
            imgui::set_tooltip("Next directory");

            ImGui::Spring(-1.0f, consts.edge_offset * 2.0f);

            if (content_browser_button("##refresh", EditorIcon::Refresh))
            {
                refresh();
            }
            imgui::set_tooltip("Refresh");

            ImGui::Spring(-1.0f, consts.edge_offset * 2.0f);
        }

        // Search
        {
            imgui::shift_cursor(0, consts.search_shift);
            ImGui::SetNextItemWidth(200);

            if (activate_search_widget)
            {
                ImGui::SetKeyboardFocusHere();
                activate_search_widget = false;
            }

            char* search_buffer_data = search_buffer.data();
            if (imgui::search_widget<MAX_INPUT_BUFFER_LENGTH>(editor_context, search_buffer_data))
            {
                if (std::strlen(search_buffer.data()) == 0)
                {
                    change_directory(current_directory);
                }
                else
                {
                    current_items = search(std::string(search_buffer.data()), current_directory);
                    sort_item_list();
                }
            }
            imgui::shift_cursor(0, -consts.search_shift);
        }

        if (update_navigation_path)
        {
            bread_crumb_data.clear();

            auto current = current_directory;
            while (current && current->parent.lock() != nullptr)
            {
                bread_crumb_data.push_back(current);
                current = current->parent.lock();
            }

            std::ranges::reverse(bread_crumb_data);
            update_navigation_path = false;
        }

        // Breadcrumbs
        {
            imgui::ScopedFont bold_font(STRING_ID("Bold"));
            auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);

            const auto& resource_directory = project.get_resource_directory();
            const auto text_size = ImGui::CalcTextSize(resource_directory.generic_string().c_str());
            const float text_padding = ImGui::GetStyle().ItemSpacing.y;

            if (ImGui::Selectable(resource_directory.generic_string().c_str(), false, 0, ImVec2(text_size.x, text_size.y + text_padding)))
            {
                SelectionSystem::deselect_all(selection_context);
                change_directory(base_directory);
            }
            update_drop_area(base_directory);

            for (auto& dir : bread_crumb_data)
            {
                ImGui::TextUnformatted("/");

                auto dir_name = dir->path.filename().string();
                auto dir_text_size = ImGui::CalcTextSize(dir_name.c_str());
                if (ImGui::Selectable(dir_name.c_str(), false, 0, ImVec2(dir_text_size.x, dir_text_size.y + text_padding)))
                {
                    SelectionSystem::deselect_all(selection_context);
                    change_directory(dir);
                }

                update_drop_area(dir);
            }
        }

        // Settings Button
        {
            ImGui::Spring();
            if (imgui::options_button(editor_context))
            {
                ImGui::OpenPopup("##content_browser_options");
            }
            imgui::set_tooltip("Content Browser Options");

            if (ImGui::BeginPopup("##content_browser_options"))
            {
                // TODO: Implement settings

                ImGui::EndPopup();
            }
        }
    }

    ImGui::EndHorizontal();
    ImGui::EndChild();
}

void ContentBrowserPanel::render_items(EditorContext& editor_context)
{
    is_any_item_hovered = false;

    for (auto& item : current_items)
    {
        item->on_render_begin();
        auto result = item->on_render(selection_context, current_items, editor_context);
        item->on_render_end();

        if (result & ActionBit::ClearSelections)
        {
            clear_selection();
        }

        if (result & ActionBit::Deselected)
        {
            SelectionSystem::deselect(item->get_resource_id(), selection_context);
        }

        if (result & ActionBit::Selected)
        {
            SelectionSystem::select(item->get_resource_id(), selection_context);
        }

        if (result & ActionBit::SelectToHere && SelectionSystem::selection_count(selection_context) == 2)
        {
            size_t first_index = current_items.find_item(SelectionSystem::get_selection_by_index(selection_context, 0));
            size_t second_index = current_items.find_item(item->get_resource_id());

            if (first_index > second_index)
            {
                std::swap(first_index, second_index);
            }

            for (size_t i = first_index + 1; i < second_index; ++i)
                SelectionSystem::select(current_items[i]->get_resource_id(), selection_context);
        }

        if (result & ActionBit::StartRenaming)
            item->start_renaming();

        if (result & ActionBit::Copy)
            copied_resources.select(item->get_resource_id());

        // TODO: implement asset reloading
        // if (result & ActionBit::Reload)
        //     //

        if (result & ActionBit::OpenDeleteDialog && !item->is_renaming())
        {
            if (item->get_type() == Item::Type::Directory)
            {
                const auto dir_item = reference_cast<DirectoryItem>(item);
                pending_removal_directory = dir_item->get_directory_info();
                open_delete_folder_popup = true;
            }
            else
            {
                open_delete_popup = true;
            }
        }


        if (result & ActionBit::ShowInExplorer)
        {
            if (item->get_type() == Item::Type::Directory)
                FileSystem::show_file_in_explorer(current_directory->path / item->get_display_name());
            else
                FileSystem::show_file_in_explorer(find_resource_path(item->get_resource_id()));
        }

        if (result & ActionBit::OpenExternal)
        {
            if (item->get_type() == Item::Type::Directory)
                FileSystem::open_externally(current_directory->path / item->get_display_name());
            else
                FileSystem::open_externally(find_resource_path(item->get_resource_id()));
        }

        if (result & ActionBit::Hovered)
            is_any_item_hovered = true;

        if (result & ActionBit::Duplicate)
        {
            copied_resources.select(item->get_resource_id());
            paste_copied_resources();
            break;
        }

        if (result & ActionBit::Renamed)
        {
            SelectionSystem::deselect_all(selection_context);
            refresh();
            sort_item_list();

            break;
        }


        if (result & ActionBit::Activated)
        {
            if (item->get_type() == Item::Type::Directory)
            {
                SelectionSystem::deselect_all(selection_context);
                change_directory(reference_cast<DirectoryItem>(item)->get_directory_info());
                break;
            }

            auto resource_item = reference_cast<ResourceItem>(item);
            auto& meta = resource_item->get_metadata();

            if (item_activation_callbacks.contains(meta.type))
            {
                item_activation_callbacks[meta.type](editor_context.resource_registry.get<Resource>(meta.resource_id));
            }
            // TODO: else, open some generic "resource editor" panel?
        }

        if (result & ActionBit::Refresh)
        {
            refresh();
            break;
        }
    }

    if (open_delete_popup)
    {
        ImGui::OpenPopup("##delete_popup");
        open_delete_popup = false;
    }

    if (open_delete_folder_popup)
    {
        ImGui::OpenPopup("##delete_folder_popup");
        open_delete_folder_popup = false;
    }
}

void ContentBrowserPanel::draw_bottombar(EditorContext&, float height)
{
    imgui::ScopedStyle child_border_size(ImGuiStyleVar_ChildBorderSize, 0.0f);
    imgui::ScopedStyle frame_border_size(ImGuiStyleVar_FrameBorderSize, 0.0f);
    imgui::ScopedStyle item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    imgui::ScopedStyle frame_padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

    ImGui::BeginChild("##bottom_bar", ImVec2(0, height));
    ImGui::BeginHorizontal("##bottom_bar");
    {
        const size_t selection_count = SelectionSystem::selection_count(selection_context);
        if (selection_count == 1)
        {
            const auto first_selection = SelectionSystem::get_selection_by_index(selection_context, 0);
            const auto index = current_items.find_item(first_selection);

            std::filesystem::path file_path = "";
            if (index != ItemList::invalid_item)
            {
                const auto& item = current_items[index];
                if (item->get_type() == Item::Type::Directory)
                {
                    const auto dir_item = reference_cast<DirectoryItem>(item);
                    file_path = dir_item->get_directory_info()->path;
                }
                else
                {
                    file_path = find_resource_path(item->get_resource_id());
                }

                if (!file_path.empty())
                {
                    file_path = std::filesystem::relative(file_path, base_directory->path);
                }
            }

            ImGui::TextUnformatted(file_path.generic_string().c_str());
        }
        else if (selection_count > 1)
        {
            ImGui::Text("%d items selected", selection_count);
        }
    }
    ImGui::EndHorizontal();
    ImGui::EndChild();
}

void ContentBrowserPanel::refresh()
{
    current_items.clear();
    directories.clear();

    auto current_dir = current_directory;
    auto base_dir_id = process_directory(project.get_resource_directory(), nullptr);
    base_directory = directories[base_dir_id];
    if (current_dir)
        current_directory = get_directory(current_dir->path);

    if (!current_directory)
        current_directory = base_directory;

    change_directory(current_directory);
}

void ContentBrowserPanel::on_key_pressed_event(const KeyPressedEvent& event)
{
    if (!is_focused)
        return;

    if (event.key == Key::LeftControl || event.modifiers & KeyModifierBits::Ctrl)
    {
        switch (event.key)
        {
        case Key::C:
            copied_resources.copy_from(SelectionSystem::get_selections(selection_context));
            return;
        case Key::V:
            paste_copied_resources();
            return;
        case Key::D:
            copied_resources.copy_from(SelectionSystem::get_selections(selection_context));
            paste_copied_resources();
            return;
        case Key::F:
            activate_search_widget = true;
            return;

        default: break;
        }
    }

    if (event.key == Key::N && event.modifiers & KeyModifierBits::Shift | KeyModifierBits::Ctrl)
    {
        auto path = current_directory->path / "New Folder";

        bool created = FileSystem::create_directory(path);

        if (created)
        {
            refresh();

            const auto dir_info = get_directory(path);
            size_t index = current_items.find_item(dir_info->id);
            if (index != ItemList::invalid_item)
            {
                SelectionSystem::deselect_all(selection_context);
                SelectionSystem::select(dir_info->id, selection_context);
                current_items[index]->start_renaming();
            }
        }
        return;
    }

    if ((event.key == Key::Right && event.modifiers & KeyModifierBits::Alt) || event.key == Key::MouseButton4)
    {
        on_browse_forward();
        return;
    }

    if ((event.key == Key::Left && event.modifiers & KeyModifierBits::Alt) || event.key == Key::MouseButton3)
    {
        on_browse_back();
        return;
    }

    if (event.key == Key::Delete)
    {
        for (const auto& item : current_items)
        {
            if (item->is_renaming())
                return;
        }

        open_delete_popup = true;
        return;
    }

    if (event.key == Key::F5)
    {
        refresh();
    }

    if ((!is_any_item_hovered && event.key == Key::LeftMouseButton) || event.key == Key::Escape)
    {
        clear_selection();
    }
}

void ContentBrowserPanel::paste_copied_resources()
{
    if (copied_resources.selection_count() == 0)
        return;

    auto get_unique_path = [](const std::filesystem::path& path)
    {
        int counter = 0;
        std::filesystem::path base_path = path;

        while (std::filesystem::exists(base_path))
        {
            ++counter;
            std::string counter_str;
            if (counter < 10)
                counter_str = "0" + std::to_string(counter);
            else
                counter_str = std::to_string(counter);
            base_path = path.parent_path() / fmt::format("{}_{}{}", path.stem().string(), counter_str, path.extension().string());
        }

        return base_path;
    };

    for (auto copied_resource : copied_resources)
    {
        const size_t index = current_items.find_item(copied_resource);

        if (index == ItemList::invalid_item)
            continue;

        const auto& item = current_items[index];
        std::filesystem::path original_path;

        if (item->get_type() == Item::Type::Resource)
        {
            original_path = std::filesystem::path(reference_cast<ResourceItem>(item)->get_metadata().full_source_path.string);
            auto new_path = get_unique_path(current_directory->path / original_path.filename());
            PORTAL_ASSERT(std::filesystem::exists(new_path), "File already exists");
            FileSystem::copy(original_path, new_path);
        }
        else
        {
            original_path = reference_cast<DirectoryItem>(item)->get_directory_info()->path;
            auto new_path = get_unique_path(current_directory->path / original_path.filename());
            PORTAL_ASSERT(std::filesystem::exists(new_path), "File already exists");
            std::filesystem::copy(original_path, new_path, std::filesystem::copy_options::recursive);
        }
    }

    refresh();

    SelectionSystem::deselect_all(selection_context);
    copied_resources.clear();
}

void ContentBrowserPanel::clear_selection()
{
    const auto selected_items = SelectionSystem::get_selections(selection_context);
    for (auto id : selected_items)
    {
        SelectionSystem::deselect(id, selection_context);
        if (const size_t index = current_items.find_item(id); index != ItemList::invalid_item)
        {
            if (current_items[index]->is_renaming())
                current_items[index]->stop_renaming();
        }
    }
}

namespace
{
    bool s_is_deleting_items = false;
    bool s_right_button_hovered = false;
    bool s_left_button_hovered = false;

    void hover_left_right_popup_buttons()
    {
        if (!s_right_button_hovered)
        {
            s_right_button_hovered = ImGui::IsKeyPressed(ImGuiKey_LeftArrow);
            s_left_button_hovered = !s_right_button_hovered;
        }

        if (!s_left_button_hovered)
        {
            s_left_button_hovered = ImGui::IsKeyPressed(ImGuiKey_RightArrow);
            s_right_button_hovered = !s_left_button_hovered;
        }
    }
}

void ContentBrowserPanel::render_delete_dialog(EditorContext&)
{
    if (ImGui::BeginPopupModal("##delete_popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (SelectionSystem::selection_count(selection_context) == 0)
            ImGui::CloseCurrentPopup();

        ImGui::Text("Are you sure you want to delete %d items?", SelectionSystem::selection_count(selection_context));

        const float content_region_width = ImGui::GetContentRegionAvail().x;
        constexpr float button_width = 60.f;

        hover_left_right_popup_buttons();

        imgui::shift_cursor(((content_region_width - (button_width * 2.0f)) / 2.0f) - ImGui::GetStyle().ItemSpacing.x, 0.f);
        if (ImGui::Button("Yes", ImVec2(button_width, 0.0f)) || (s_right_button_hovered && ImGui::IsKeyPressed(ImGuiKey_Enter)))
        {
            s_is_deleting_items = true;

            auto selected_items = SelectionSystem::get_selections(selection_context);
            for (auto id : selected_items)
            {
                size_t index = current_items.find_item(id);
                if (index == ItemList::invalid_item)
                    continue;

                // TODO: mark resources for deletion
            }

            // TODO: call deletion callbacks

            SelectionSystem::deselect_all(selection_context);
            refresh();

            s_is_deleting_items = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        ImGui::SetItemDefaultFocus();
        if (ImGui::Button("No", ImVec2(button_width, 0.0f)) || (s_left_button_hovered && ImGui::IsKeyPressed(ImGuiKey_Enter)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void ContentBrowserPanel::render_delete_folder_dialog(EditorContext&)
{
    if (ImGui::BeginPopupModal("##delete_folder_popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (pending_removal_directory == nullptr)
            ImGui::CloseCurrentPopup();

        ImGui::Text("Are you sure you want to delete %s", pending_removal_directory->path.filename().string().c_str());

        const float content_region_width = ImGui::GetContentRegionAvail().x;
        constexpr float button_width = 60.f;

        hover_left_right_popup_buttons();

        imgui::shift_cursor(((content_region_width - (button_width * 2.0f)) / 2.0f) - ImGui::GetStyle().ItemSpacing.x, 0.f);
        if (ImGui::Button("Yes", ImVec2(button_width, 0.0f)) || (s_right_button_hovered && ImGui::IsKeyPressed(ImGuiKey_Enter)))
        {
            if (delete_directory(*pending_removal_directory))
            {
                SelectionSystem::deselect_all(selection_context);

                if (current_directory == pending_removal_directory)
                {
                    auto parent = current_directory->parent.lock()->path;
                    if (parent.empty())
                    {
                        change_directory(base_directory);
                    }
                    else
                    {
                        for (auto& dir : directories | std::views::values)
                        {
                            if (dir->path == parent)
                            {
                                change_directory(dir);
                                break;
                            }
                        }
                    }
                }

                refresh();
                pending_removal_directory = nullptr;
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        ImGui::SetItemDefaultFocus();
        if (ImGui::Button("No", ImVec2(button_width, 0.0f)) || (s_left_button_hovered && ImGui::IsKeyPressed(ImGuiKey_Enter)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void ContentBrowserPanel::remove_directory_info(const Reference<DirectoryInfo>& directory, const bool remove_from_parent)
{
    if (!directory->parent.expired() && directory->parent.lock() && remove_from_parent)
    {
        auto& child_list = directory->parent.lock()->subdirectories;
        child_list.erase(child_list.find(directory->id));
    }

    for (const auto& subdir : directory->subdirectories | std::views::values)
    {
        remove_directory_info(subdir, false);
    }

    directory->subdirectories.clear();
    directory->resources.clear();

    directories.erase(directories.find(directory->id));
}

void ContentBrowserPanel::update_drop_area(const Reference<DirectoryInfo> target)
{
    if (target->id != current_directory->id && ImGui::BeginDragDropTarget())
    {
        const auto* payload = ImGui::AcceptDragDropPayload("resource_payload");

        if (payload)
        {
            auto count = payload->DataSize / sizeof(StringId);

            for (size_t i = 0; i < count; ++i)
            {
                auto id = *(static_cast<const StringId*>(payload->Data) + i);
                auto index = current_items.find_item(id);
                if (index != ItemList::invalid_item)
                {
                    current_items[index]->move(target->path);
                    current_items.erase(id);
                }
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void ContentBrowserPanel::sort_item_list()
{
    std::ranges::sort(
        current_items,
        [](const Reference<Item>& item1, const Reference<Item>& item2)
        {
            if (item1->get_type() == item2->get_type())
                return to_lower_copy(item1->get_display_name()) < to_lower_copy(item2->get_display_name());

            return static_cast<uint8_t>(item1->get_type()) > static_cast<uint8_t>(item2->get_type());
        }
    );
}

std::filesystem::path ContentBrowserPanel::find_resource_path(const StringId& resource_id) const
{
    auto meta = project.get_resource_database().find(resource_id);
    if (meta.has_value())
        return meta->full_source_path.string;
    return "";
}

ItemList ContentBrowserPanel::search(const std::string& query, const Reference<DirectoryInfo>& directory)
{
    ItemList result;
    const auto query_lower_case = to_lower_copy(query);

    for (auto& subdir : directory->subdirectories | std::views::values)
    {
        auto subdir_name = subdir->path.filename().string();
        if (subdir_name.find(query_lower_case) != std::string::npos)
            result.items.push_back(make_reference<DirectoryItem>(subdir));

        ItemList list = search(query, subdir);
        result.items.insert(result.items.end(), list.items.begin(), list.items.end());
    }

    for (auto& id : directory->resources)
    {
        auto meta = project.get_resource_database().find(id);
        auto filename = to_lower_copy(std::filesystem::path(meta->full_source_path.string).filename().string());

        if (filename.find(query_lower_case) != std::string::npos)
            result.items.push_back(make_reference<ResourceItem>(meta.value(), EditorIcon::File));
    }

    return result;
}
} // portal

