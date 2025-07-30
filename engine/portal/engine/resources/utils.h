//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resource_types.h"
#include "portal/core/reference.h"
#include "portal/engine/resources/resources/resource.h"

namespace portal::resources::utils
{
template<class T>
ResourceType to_resource_type()
{
    if constexpr (std::is_same_v<T, Texture>)
        return ResourceType::Texture;
    return ResourceType::Unknown;
}

ResourceType to_resource_type(const Ref<Resource>& resource);

Ref<Resource> create_resource(const StringId& id, ResourceType type);
}
