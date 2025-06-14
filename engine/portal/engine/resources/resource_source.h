//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/buffer.h"

namespace portal::resources
{

class ResourceSource
{
public:
    virtual ~ResourceSource() = default;
    virtual void load(Buffer buffer) = 0;
};

} // portal
