//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal::renderer
{

// A base type for type erased storage for descriptors.
class BaseDescriptor
{
public:
    virtual ~BaseDescriptor() = default;
};

}