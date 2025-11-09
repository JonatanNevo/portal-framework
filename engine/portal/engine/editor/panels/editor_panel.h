//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <portal/engine/resources/resource_reference.h>
#include <portal/engine/scene/scene.h>


namespace portal::editor
{
class EditorPanel
{
public:
    virtual ~EditorPanel() = default;

    virtual void on_gui_render() = 0;
    virtual void set_scene_context(const ResourceReference<Scene>& scene) = 0;
};
};
