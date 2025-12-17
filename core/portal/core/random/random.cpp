//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "random.h"

namespace portal
{
glm::vec3 Random::get_vec3()
{
    return {get_float(), get_float(), get_float()};
}

glm::vec3 Random::get_vec3(const float min, const float max)
{
    return {get_float(min, max), get_float(min, max), get_float(min, max)};
}
} // namespace portal
