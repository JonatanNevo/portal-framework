//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include "resource_database.h"

#include <ranges>

namespace portal
{

void SourceMetadata::archive(ArchiveObject& archive) const
{
    archive.add_property("resource_id", resource_id.string);
    archive.add_property("type", utils::to_string(type));
    archive.add_property("dependencies", dependencies | std::ranges::views::transform([](const auto& id) { return id.string; }) | std::ranges::to<std::vector>()); // TODO: support views

    archive.add_property("source", source.string);
    archive.add_property("format", utils::to_string(format));
}

SourceMetadata SourceMetadata::dearchive(ArchiveObject& archive)
{
    std::string resource_name;
    std::string type_string;

    std::string source;
    std::string format_string;
    std::vector<std::string> dependencies;

    archive.get_property<std::string>("resource_id", resource_name);
    archive.get_property<std::string>("type", type_string);
    archive.get_property<std::vector<std::string>>("dependencies", dependencies);

    archive.get_property<std::string>("source", source);
    archive.get_property<std::string>("format", format_string);

    auto string_id_view = dependencies | std::ranges::views::transform([](const auto& id) { return STRING_ID(id); });
    return {
        STRING_ID(resource_name),
        utils::to_resource_type(type_string),
        string_id_view | std::ranges::to<llvm::SmallVector<StringId>>(),
        STRING_ID(source),
        utils::to_source_format(format_string)
    };
}

} // portal
