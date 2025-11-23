//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/application/modules/module.h"
#include "portal/core/events/event_handler.h"
#include "portal/engine/renderer/camera.h"
#include "portal/engine/resources/resource_reference.h"
#include "portal/engine/scene/scene.h"

namespace portal
{
class WindowResizeEvent;
class Renderer;

// TODO: consolidate with some "Script Manager"?
class SceneManager final : public TaggedModule<Tag<ModuleTags::Update, ModuleTags::Event, ModuleTags::GuiUpdate>, Renderer, ResourceRegistry, Input>,
                           public EventHandler
{
public:
    explicit SceneManager(ModuleStack& stack);

    void set_active_scene(const ResourceReference<Scene>& new_scene);

    void update(FrameContext& frame) override;
    void gui_update(FrameContext& frame) override;

    void on_event(Event& event) override;

protected:
    void on_resize(const WindowResizeEvent& event);

private:
    void print_scene_graph();

private:
    // TODO:: find a better place for this
    Camera camera;
    ResourceReference<Scene> active_scene;
};
} // portal
