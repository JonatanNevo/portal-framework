//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal::renderer
{

class Sampler
{
public:
    virtual ~Sampler() = default;

    [[nodiscard]] virtual const SamplerSpecification& get_spec() const = 0;
};

}
