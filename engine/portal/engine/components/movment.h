//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <glm/vec3.hpp>

#include "register_component.h"

namespace portal
{
struct SpeedComponent
{
    glm::vec3 speed = glm::vec3(0.0f);
};
}
