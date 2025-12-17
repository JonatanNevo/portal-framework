//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resource_types.h"
#include "portal/engine/resources/resources/resource.h"


namespace portal::utils
{
template <class T>
ResourceType to_resource_type()
{
    return T::static_type();
}

ResourceType to_resource_type(const Resource& resource);
}
