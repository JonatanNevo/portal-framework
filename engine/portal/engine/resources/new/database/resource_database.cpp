//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include "portal/engine/resources/new/database/resource_database.h"

namespace portal::ng
{

void SourceMetadata::archive(ArchiveObject& archive) const
{
    archive.add_property("resource_id", resource_id.string);
    archive.add_property("handle", handle);
    archive.add_property("type", utils::to_string(type));
    archive.add_property("dependencies", dependencies);

    archive.add_property("source", source.string);
    archive.add_property("format", utils::to_string(format));
}

SourceMetadata SourceMetadata::dearchive(ArchiveObject& archive)
{
    std::string resource_name;
    uint64_t handle;
    std::string type_string;

    std::string source;
    std::string format_string;
    llvm::SmallVector<ResourceHandle> dependencies;

    archive.get_property<uint64_t>("handle", handle);
    archive.get_property<std::string>("resource_id", resource_name);
    archive.get_property<std::string>("type", type_string);
    archive.get_property<llvm::SmallVector<ResourceHandle>>("dependencies", dependencies);

    archive.get_property<std::string>("source", source);
    archive.get_property<std::string>("format", format_string);

    return {
        {handle, resource_name},
        handle,
        utils::to_resource_type(type_string),
        dependencies,
        STRING_ID(source),
        utils::to_source_format(format_string)
    };
}

} // portal
