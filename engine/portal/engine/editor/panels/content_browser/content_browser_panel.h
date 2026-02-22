//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "items/directory_item.h"
#include "items/item_list.h"
#include "portal/engine/ecs/entity.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/editor/panels/panel.h"
#include "portal/engine/project/project.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/database/resource_database_facade.h"
#include "portal/input/input_events.h"

namespace portal
{
namespace content_browser
{
    class SelectionStack
    {
    public:
        void copy_from(SelectionStack& other);
        void copy_from(const std::vector<StringId>& other);

        void select(const StringId& id);
        void deselect(const StringId& id);

        [[nodiscard]] bool is_selected(const StringId& id) const;
        void clear();

        size_t selection_count() const;
        const StringId* selection_data() const;

        StringId operator[](size_t index) const;

        std::vector<StringId>::iterator begin();
        std::vector<StringId>::iterator end();
        std::vector<StringId>::const_iterator begin() const;
        std::vector<StringId>::const_iterator end() const;

    private:
        std::vector<StringId> selections;
    };
}


class ContentBrowserPanel final : public Panel
{
public:
    ContentBrowserPanel(const EditorContext& editor_context);

    Entity get_selection_context() const { return selection_context; }

    void on_gui_render(EditorContext& editor_context, FrameContext& frame_context) override;

    content_browser::ItemList& get_current_items() { return current_items; }
    Reference<content_browser::DirectoryInfo> get_directory(const std::filesystem::path& path) const;

    void register_item_activation_callback(StringId id, std::function<void(const ResourceReference<Resource>)> callback);
    void register_resource_created_callback(std::function<void(const ResourceReference<Resource>)> callback);
    void register_resource_deleted_callback(std::function<void(const ResourceReference<Resource>)> callback);

    bool delete_directory(const content_browser::DirectoryInfo& info);

private:
    StringId process_directory(const resources::DatabaseEntry& entry, const std::filesystem::path& root_path, const Reference<content_browser::DirectoryInfo>& parent);

    void change_directory(const Reference<content_browser::DirectoryInfo>& directory);
    void on_browse_back();
    void on_browse_forward();

    void render_directory_tree(EditorContext& editor_context, Reference<content_browser::DirectoryInfo> directory);
    void draw_topbar(EditorContext& editor_context, float height);
    void render_items(EditorContext& editor_context);
    void draw_bottombar(EditorContext& editor_context, float height);

    void refresh();

    void on_key_pressed_event(const KeyPressedEvent& event);

    void paste_copied_resources();

    void clear_selection();

    void render_delete_dialog(EditorContext& editor_context);
    void render_delete_folder_dialog(EditorContext& editor_context);

    void remove_directory_info(const Reference<content_browser::DirectoryInfo>& directory, bool remove_from_parent = true);

    void update_drop_area(const Reference<content_browser::DirectoryInfo> target);

    void sort_item_list();

    std::filesystem::path find_resource_path(const StringId& resource_id) const;

    content_browser::ItemList search(const std::string& query, const Reference<content_browser::DirectoryInfo>& directory);

    template <typename T, typename... Args>
    ResourceReference<T> create_resource(std::filesystem::path filename, EditorContext& editor_context, Args&&... args)
    {
        return create_resource_in_directory<T>(filename, editor_context, current_directory, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    ResourceReference<T> create_resource_in_directory(
        const std::filesystem::path,
        EditorContext&,
        const Reference<content_browser::DirectoryInfo>,
        Args&&...
    )
    {
        LOG_WARN_TAG("Content Browser", "Resource importing is not yet implemented, skipping...");
        return ResourceReference<T>();
        // auto abs_filename = directory->path / filename;
        //
        // auto& database = editor_context.project.get_resource_database();
        //
        // const auto resource_id = StringId{};
        // // TODO: add a generic "add resource from file" to the database
        // const auto result = database.add(resource_id, SourceMetadata{});
        // if (result != DatabaseErrorBit::Success)
        //     return ResourceReference<T>();
        //
        // directory->resources.push_back(resource_id);
        // ResourceReference<T> resource = editor_context.resource_registry.immediate_load<T>(resource_id);
        // for (auto& callback : new_resource_created_callbacks)
        //     callback(resource.template cast<Resource>());
        //
        // refresh();
        //
        // return resource;
    }

private:
    // TODO: generate and cache thumbnails

    Entity selection_context;
    Project& project;

    content_browser::ItemList current_items;

    Reference<content_browser::DirectoryInfo> current_directory;
    std::vector<Reference<content_browser::DirectoryInfo>> base_directories;
    Reference<content_browser::DirectoryInfo> next_directory, prev_directory;
    Reference<content_browser::DirectoryInfo> pending_removal_directory;

    bool is_any_item_hovered = false;

    content_browser::SelectionStack copied_resources;

    std::unordered_map<StringId, Reference<content_browser::DirectoryInfo>> directories;

    std::unordered_map<ResourceType, std::function<void(const ResourceReference<Resource>)>> item_activation_callbacks;
    std::vector<std::function<void(const ResourceReference<Resource>)>> new_resource_created_callbacks;
    std::vector<std::function<void(const ResourceReference<Resource>)>> resource_deleted_callbacks;

    std::array<char, content_browser::MAX_INPUT_BUFFER_LENGTH> search_buffer{};
    // TODO: should this be a linked list instead?
    std::vector<Reference<content_browser::DirectoryInfo>> bread_crumb_data;

    bool update_navigation_path = true;

    bool activate_search_widget = false;
    bool open_delete_popup = false;
    bool open_delete_folder_popup = false;

    bool is_hovered = false;
    bool is_focused = false;
};
} // portal
