//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "resource.h"

namespace portal
{

struct FontProperties
{
    StringId name;
    std::filesystem::path path;
    // I wanted this to be wchar_t but on mac its 4 bytes instead of 2 :(
    std::optional<std::array<uint16_t, 3>> glyph_range = std::nullopt;
};

// TODO: This is a placeholder class for a font, Currently I dont have font rendering in game, so this is used only for imgui.
class Font final : public Resource
{
public:
    explicit Font(const StringId& id, const FontProperties& properties) : Resource(id), properties(properties) {}
    [[nodiscard]] const FontProperties& get_properties() const { return properties; }

private:
    FontProperties properties;

};
} // portal