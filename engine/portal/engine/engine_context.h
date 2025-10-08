//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/renderer.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/application/window.h"

namespace portal
{

struct EngineContext
{
    Renderer* renderer;
    ResourceRegistry* resource_registry;
    Window* window;
};

}
