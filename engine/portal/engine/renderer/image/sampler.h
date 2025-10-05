//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/reference.h"

namespace portal::renderer
{

class Sampler: public RefCounted
{
public:
    [[nodiscard]] virtual const SamplerSpecification& get_spec() const = 0;
};

}
