//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal::renderer
{
/**
 * @class Sampler
 * @brief Abstract sampler interface for texture filtering
 */
class Sampler
{
public:
    virtual ~Sampler() = default;

    /** @brief Gets sampler properties */
    [[nodiscard]] virtual const SamplerProperties& get_prop() const = 0;
};
}
