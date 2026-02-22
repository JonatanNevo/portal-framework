//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vector>

#include "register_component.h"
#include "portal/core/strings/string_id.h"

namespace portal
{

struct SelectionComponent
{
    std::vector<StringId> selections;
};
}
