//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resource_reference.h"
#include "portal/engine/scene/scene.h"

namespace portal
{
struct SceneContext
{
    ResourceReference<Scene> active_scene;
};
}
