//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <imgui.h>

#include "panels/panel.h"
#include "portal/engine/ecs/system.h"
#include "portal/engine/imgui/utils.h"

namespace portal
{
struct EditorContext;

struct PanelData
{
    StringId id;
    const char* name;
    Reference<Panel> panel = nullptr;
    bool open = false;

    void archive(ArchiveObject& object) const;
    static PanelData dearchive(ArchiveObject& object);
};

enum class PanelMenuCategory
{
    Edit,
    View
};

class PanelManager
{
public:
    PanelManager(std::filesystem::path state_path);

    PanelData* get_panel_data(const StringId& id);
    const PanelData* get_panel_data(const StringId& id) const;

    template <typename T> requires std::is_base_of_v<Panel, T>
    Reference<T> add_panel(const PanelMenuCategory category, const PanelData& panel_data)
    {
        auto& panel_map = panels[std::to_underlying(category)];
        if (panel_map.contains(panel_data.id))
        {
            LOG_ERROR_TAG("Panel Manager", "A panel with id '{}' already exists", panel_data.id);
            return nullptr;
        }

        panel_map.emplace(panel_data.id, panel_data);
        return reference_cast<T>(panel_data.panel);
    }

    template <typename T, typename... Args> requires std::is_base_of_v<Panel, T>
    Reference<T> add_panel(const PanelMenuCategory category, const StringId id, const char* name, bool open_by_default, Args&&... args)
    {
        return add_panel<T>(category, PanelData{id, name, make_reference<T>(std::forward<Args>(args)...), open_by_default});
    }

    template <typename T, typename... Args> requires std::is_base_of_v<Panel, T>
    Reference<T> add_panel(const PanelMenuCategory category, StringId id, bool open_by_default, Args&&... args)
    {
        return add_panel<T>(category, id, id.string.data(), open_by_default, std::forward<Args>(args)...);
    }

    template <typename T> requires std::is_base_of_v<Panel, T>
    Reference<T> get_panel(StringId id)
    {
        for (const auto& map : panels)
        {
            if (map.contains(id))
                return map.at(id).panel;
        }

        LOG_ERROR_TAG("Panel Manager", "Panel with id '{}' not found", id);
        return nullptr;
    }

    void remove_panel(StringId id);

    /** @brief Main execution entry point, renders all editor panels. */
    void on_gui_render(EditorContext& editor_context, FrameContext& frame);

    [[nodiscard]] std::unordered_map<StringId, PanelData>& get_panels(PanelMenuCategory category);
    [[nodiscard]] const std::unordered_map<StringId, PanelData>& get_panels(PanelMenuCategory category) const;

    void save_state();
    void load_state();

private:
    std::filesystem::path state_path;
    std::array<std::unordered_map<StringId, PanelData>, enchantum::count<PanelMenuCategory>> panels;
};
} // portal
