//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/engine/entry_point.h"
#include "portal/engine/engine.h"

using namespace portal;

std::unique_ptr<Application> portal::create_engine_application(Reference<Project>&& project, int, char**)
{
    const ApplicationProperties prop = from_project(*project);
    auto engine = std::make_unique<Engine>(project, prop);

    return engine;
}
