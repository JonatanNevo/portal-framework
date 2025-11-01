//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include "portal/engine/application/application.h"
using namespace portal;

void add_resource(const std::filesystem::path& path, ResourceDatabase& database)
{
    auto extension = path.extension();
    auto value = utils::find_extension_type(extension.string());
    if (!value.has_value())
        return;
    auto [resource_type, source_format] = value.value();

    const auto id = STRING_ID(path.filename().replace_extension("").string());
    SourceMetadata meta{
        .resource_id = id,
        .handle = id.id,
        .type = resource_type,
        .source = STRING_ID(path.string()),
        .format = source_format,
    };

    database.add(meta);
}

int main()
{
    // Log::init();

    // FolderResourceDatabase database();
    // database.remove(STRING_ID("fish_texture"));
    // StringId resource = STRING_ID("fish_texture");

    ApplicationSpecification spec{
        .resources_path = R"(C:\Users\thejo\OneDrive\Documents\PortalEngine\test)"
    };
    Application app{spec};
    app.run();
}
