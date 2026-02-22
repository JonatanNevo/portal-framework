//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "editor_icons.h"
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
    EditorIcons& icons;
    ecs::Registry& ecs_registry;
    ResourceRegistry& resource_registry;
    InputManager& input_manager;

    entt::delegate<void()> restore_default_settings;
};

}
