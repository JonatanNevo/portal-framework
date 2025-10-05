//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/resources/resources/resource.h"

namespace portal::renderer
{

class RendererResource: public Resource
{
public:
    explicit RendererResource(const StringId& id): Resource(id) {}
};

}
