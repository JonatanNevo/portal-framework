//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/modules/system_orchestrator.h"

namespace portal
{

class InputRouter
{
public:
    InputRouter(
        SystemOrchestrator& orchestrator,
        entt::dispatcher& engine_dispatcher,
        entt::dispatcher& input_dispatcher
        );

    void block_input() const;
    void unblock_input() const;

private:
    SystemOrchestrator& orchestrator;
    entt::dispatcher& engine_dispatcher;
    entt::dispatcher& input_dispatcher;
};

} // portal