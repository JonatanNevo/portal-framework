//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "input_router.h"

#include <imgui.h>

namespace portal
{
InputRouter::InputRouter(SystemOrchestrator& orchestrator, entt::dispatcher& engine_dispatcher, entt::dispatcher& input_dispatcher)
    : orchestrator(orchestrator), engine_dispatcher(engine_dispatcher), input_dispatcher(input_dispatcher) {}

void InputRouter::block_input() const
{
    engine_dispatcher.trigger(SetMouseCursorEvent{CursorMode::Normal});
    orchestrator.disconnect(input_dispatcher);

    auto& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavNoCaptureKeyboard;
}

void InputRouter::unblock_input() const
{
    engine_dispatcher.trigger(SetMouseCursorEvent{CursorMode::Locked});
    orchestrator.connect(input_dispatcher);

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;
}
} // portal
