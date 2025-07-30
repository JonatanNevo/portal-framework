//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vector>

#include "portal/engine/renderer/scene/render_object.h"

namespace portal
{


struct DrawContext
{
    std::vector<RenderObject> opaque_surfaces;
    std::vector<RenderObject> transparent_surfaces;
};

} // portal
