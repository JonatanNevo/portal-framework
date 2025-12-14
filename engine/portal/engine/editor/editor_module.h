//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "editor_system.h"
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/renderer.h"

namespace portal
{

class EditorModule final : public TaggedModule<Tag<ModuleTags::GuiUpdate>, Renderer, SystemOrchestrator>
{
public:
    explicit EditorModule(ModuleStack& stack);

    void gui_update(FrameContext& frame) override;

private:
    EditorGuiSystem gui_system;
};

} // portal