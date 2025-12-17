//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "editor_module.h"

namespace portal
{
EditorModule::EditorModule(ModuleStack& stack) : TaggedModule(stack, STRING_ID("Editor Module")) {}

void EditorModule::gui_update(FrameContext& frame)
{
    gui_system.execute(*frame.ecs_registry);
}
} // portal
