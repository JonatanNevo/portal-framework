//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/engine_context.h"

namespace portal
{

class ImGuiModule
{
public:
    ImGuiModule(const std::shared_ptr<EngineContext>& context);
    ~ImGuiModule();

    void begin();
    void end();

    void on_gui_render();

private:
    vk::raii::DescriptorPool imgui_pool = nullptr;
    std::shared_ptr<EngineContext> context;
};
} // portal
