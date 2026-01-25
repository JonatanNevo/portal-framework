//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "font_loader.h"

#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/resources/font.h"

namespace portal::resources
{
FontLoader::FontLoader(ResourceRegistry& registry) : ResourceLoader(registry) {}

ResourceData FontLoader::load(const SourceMetadata& meta, Reference<ResourceSource> source)
{
    if (meta.format != SourceFormat::FontFile)
    {
        PORTAL_ASSERT(0, "Cannot read font that is not from file");
        return {};
    }

    const auto& [name, glyph_range_min, glyph_range_max] = std::get<FontMetadata>(meta.meta);
    FontProperties font_properties{
        name,
        std::filesystem::path(meta.full_source_path.string)
    };

    if (glyph_range_max != 0 && glyph_range_min != 0)
        font_properties.glyph_range = {glyph_range_min, glyph_range_max, 0};

    return {make_reference<Font>(meta.resource_id, font_properties), source, meta};
}

void FontLoader::enrich_metadata(SourceMetadata& meta, const ResourceSource&)
{
    // TODO: Read the .ttf file somehow?
    meta.meta = FontMetadata{};
}

void FontLoader::save(ResourceData&) {}
} // portal
