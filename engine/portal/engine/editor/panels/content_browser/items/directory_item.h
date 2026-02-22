//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "item.h"

namespace portal::content_browser
{

struct DirectoryInfo
{
    StringId id;
    WeakReference<DirectoryInfo> parent;

    std::filesystem::path path;
    std::string display_name;
    std::vector<StringId> resources;

    std::unordered_map<StringId, Reference<DirectoryInfo>> subdirectories;

    [[nodiscard]] std::string get_display_name() const
    {
        return display_name.empty() ? path.filename().string() : display_name;
    }
};

class DirectoryItem final : public Item
{
public:
    explicit DirectoryItem(const Reference<DirectoryInfo>& directory_info);

    Reference<DirectoryInfo>& get_directory_info() { return directory_info; }

    void del() override;
    bool move(std::filesystem::path& destination) override;

protected:
    void on_renamed(const std::string& new_name) override;
    void update_drop(ItemList& item_list, Action& result) override;

private:
    Reference<DirectoryInfo> directory_info;
};

} // portal