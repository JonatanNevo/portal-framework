//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource.h"

namespace portal
{
bool Resource::operator==(const Resource& other) const
{
    return id == other.id;
}
} // portal
