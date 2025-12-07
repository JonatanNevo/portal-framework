//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "portal/core/uuid.h"
#include "portal/core/strings/string_id.h"

namespace portal
{
struct TagComponent
{
    StringId tag;
};
}
