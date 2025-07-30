//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>

#include "portal/engine/strings/string_id.h"
#include "../resources/resource.h"

namespace portal::resources
{

class ResourceSource;

class ResourceDatabase
{

public:
    virtual ~ResourceDatabase() = default;

    [[nodiscard]] virtual std::shared_ptr<ResourceSource> get_source(StringId id) const = 0;
};
}
