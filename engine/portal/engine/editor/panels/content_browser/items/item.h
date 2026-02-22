//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include <portal/core/flags.h>

#include "portal/core/strings/string_id.h"
#include "portal/engine/editor/editor_icons.h"
#include "portal/engine/resources/resource_reference.h"

namespace portal
{
struct EditorContext;
}

namespace portal::content_browser
{
class ItemList;

constexpr static auto MAX_INPUT_BUFFER_LENGTH = 128;

enum class ActionBit : uint16_t
{
    None             = 0b0000000000000000,
    Refresh          = 0b0000000000000001,
    ClearSelections  = 0b0000000000000010,
    Selected         = 0b0000000000000100,
    Deselected       = 0b0000000000001000,
    Hovered          = 0b0000000000010000,
    Renamed          = 0b0000000000100000,
    OpenDeleteDialog = 0b0000000001000000,
    SelectToHere     = 0b0000000010000000,
    Moved            = 0b0000000100000000,
    ShowInExplorer   = 0b0000001000000000,
    OpenExternal     = 0b0000010000000000,
    Reload           = 0b0000100000000000,
    Copy             = 0b0001000000000000,
    Duplicate        = 0b0010000000000000,
    StartRenaming    = 0b0100000000000000,
    Activated        = 0b1000000000000000,
};
}

template <>
struct portal::FlagTraits<portal::content_browser::ActionBit>
{
    static constexpr bool is_bitmask = true;
    static constexpr auto all_flags = Flags<portal::content_browser::ActionBit>(std::numeric_limits<std::underlying_type_t<portal::content_browser::ActionBit>>::max());
};

namespace portal::content_browser
{
using Action = Flags<ActionBit>;

class Item
{
public:
    enum class Type: uint8_t
    {
        Directory,
        Resource // TODO: add composite resource?
    };

public:
    Item(Type type, const StringId& resource_id, const std::string& name, const EditorIcon& icon);
    virtual ~Item() = default;

    void on_render_begin() const;
    Action on_render(Entity selection_context, ItemList& item_list, EditorContext& editor_context);
    void on_render_end() const;

    virtual void del() {};
    virtual bool move(std::filesystem::path& destination);

    [[nodiscard]] StringId get_resource_id() const { return resource_id; }
    [[nodiscard]] Type get_type() const { return type; }
    const std::string& get_display_name() const { return display_name; }

    [[nodiscard]] EditorIcon get_icon() const { return icon; }

    void start_renaming();
    void stop_renaming();
    bool is_renaming() const { return renaming; }

    void rename(const std::string& new_name);
    void set_display_name_from_file_name();

protected:
    virtual void on_renamed(const std::string& new_name) { file_name = new_name; }
    virtual void render_custom_context_items() {}
    virtual void update_drop(ItemList&, Action&) {}

    void on_context_menu_open(Entity selection_context, Action& result);

protected:
    Type type{};
    StringId resource_id;
    std::string display_name;
    std::string file_name;

    EditorIcon icon;

    bool renaming = false;
    bool dragging = false;
    bool just_selected = false;
};

inline bool Item::move(std::filesystem::path&)
{
    return false;
}
}
