//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "utils.h"

namespace portal::resources::utils
{

ResourceType to_resource_type(const Resource& resource)
{
    return resource.static_type();
}
}
