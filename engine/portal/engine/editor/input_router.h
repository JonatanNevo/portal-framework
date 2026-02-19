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

    void block_input();
    void unblock_input();

    [[nodiscard]] bool is_input_blocked() const { return input_blocked; }

private:
    bool input_blocked = false;
    SystemOrchestrator& orchestrator;
    entt::dispatcher& engine_dispatcher;
    entt::dispatcher& input_dispatcher;
};

} // portal