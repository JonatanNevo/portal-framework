//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "snapshot_manager.h"
#include "portal/engine/imgui/theme/editor_theme.h"
#include "portal/engine/window/window.h"

namespace portal
{

struct EditorContext
{
    imgui::EditorTheme theme;
    SnapshotManager snapshot_manager;
    Window& window;
    entt::dispatcher& engine_dispatcher;
    Project& project;

    entt::delegate<void()> restore_default_settings;
};

}
