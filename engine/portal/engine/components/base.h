//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/uuid.h"
#include "portal/core/strings/string_id.h"

namespace portal
{
struct NameComponent
{
    StringId name;
};

struct PlayerTag
{
    const int id = 0;
};

}
