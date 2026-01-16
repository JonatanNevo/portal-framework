//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "loader.h"

namespace portal::resources
{

class FontLoader final: public ResourceLoader
{
public:
    explicit FontLoader(ResourceRegistry& registry);

    Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;
    static void enrich_metadata(SourceMetadata& meta, const ResourceSource& source);
};

} // portal