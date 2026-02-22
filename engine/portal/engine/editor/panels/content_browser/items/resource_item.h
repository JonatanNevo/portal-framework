//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "item.h"

namespace portal::content_browser
{

class ResourceItem final: public Item
{
public:
    ResourceItem(const SourceMetadata& metadata, EditorIcon icon);

    [[nodiscard]] const SourceMetadata& get_metadata() const { return metadata; } [[maybe_unused]]

    void del() override;
    bool move(std::filesystem::path& destination) override;

protected:
    void on_renamed(const std::string& new_name) override;

private:
    SourceMetadata metadata;

};

} // portal