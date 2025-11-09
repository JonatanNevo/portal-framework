//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "editor_panel.h"

namespace portal::editor
{

class SceneGraphPanel final: public EditorPanel
{
public:
    SceneGraphPanel() = default;

    void on_gui_render() override;

    void set_scene_context(const ResourceReference<Scene>& new_scene) override;

protected:
    void draw_node(const Reference<scene::Node>& node) const;

private:
    ResourceReference<Scene> scene;
    bool is_window = true;
    bool focused = false;

};

}