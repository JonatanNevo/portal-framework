//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>

#include "portal/engine/strings/string_id.h"
#include "portal/engine/resources/resource.h"

namespace portal::resources
{

class ResourceSource;

class ResourceDatabase
{

public:
    virtual ~ResourceDatabase() = default;

    virtual Resource* get_default_resource(StringId id) const = 0;
    virtual std::weak_ptr<ResourceSource> get_source(StringId id) const = 0;
};
}
