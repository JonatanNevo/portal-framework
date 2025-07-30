//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/glm.h>

namespace portal
{

struct DrawContext;

class Renderable
{
public:
    virtual ~Renderable() = default;

    virtual void draw(const glm::mat4& top_matrix, DrawContext& context) = 0;
};

} // portal
