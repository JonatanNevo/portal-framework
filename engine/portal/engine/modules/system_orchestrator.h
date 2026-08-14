//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>

#include "scheduler_module.h"
#include "portal/application/modules/module.h"
#include "../../../../input/portal/input/old/input_manager.h"
#include "portal/engine/ecs/registry.h"
#include "portal/engine/scene/scene.h"
#include "portal/engine/systems/base_camera_system.h"
#include "portal/engine/systems/base_player_input_system.h"
#include "portal/engine/systems/scene_rendering_system.h"
#include "portal/engine/systems/transform_hierarchy_system.h"

namespace portal
{
enum class SystemPhase
{
    Input,
    FixedUpdate,
    Update,
    PreRender
};

// TODO: support dynamic dependencies between systems instead of fixed backets
// TODO: support creating system stack from file
class SystemOrchestrator final : public TaggedModule<
        Tag<ModuleTags::Update, ModuleTags::FixedUpdate, ModuleTags::PostUpdate, ModuleTags::FrameLifecycle>,
        ecs::Registry,
        SchedulerModule,
        InputManager
    >
{
public:
    explicit SystemOrchestrator(ModuleStack& stack);
    void clean();

    template <typename S, typename... Args>
    S& add_system(const SystemPhase phase, Args&&... args)
    {
        auto& registry = get_dependency<ecs::Registry>();

        switch (phase)
        {
        case SystemPhase::Input:
            {
                auto s_ptr = std::make_unique<S>(std::forward<Args>(args)...);
                s_ptr->register_to(registry);
                auto& s = input_systems.emplace_back(std::move(s_ptr));
                return dynamic_cast<S&>(*s);

            }

        case SystemPhase::FixedUpdate:
            {
                auto s_ptr = std::make_unique<S>(std::forward<Args>(args)...);
                s_ptr->register_to(registry);
                auto& s = fixed_update_systems.emplace_back(std::move(s_ptr));
                return dynamic_cast<S&>(*s);

            }
        case SystemPhase::Update:
            {
                auto s_ptr = std::make_unique<S>(std::forward<Args>(args)...);
                s_ptr->register_to(registry);
                auto& s = update_systems.emplace_back(std::move(s_ptr));
                return dynamic_cast<S&>(*s);

            }
        case SystemPhase::PreRender:
            {
                auto s_ptr = std::make_unique<S>(std::forward<Args>(args)...);
                s_ptr->register_to(registry);
                auto& s = pre_render_systems.emplace_back(std::move(s_ptr));
                return dynamic_cast<S&>(*s);
            }
        }

        throw std::runtime_error("Invalid phase");
    }

    void connect(entt::dispatcher& dispatcher);
    void disconnect(entt::dispatcher& dispatcher);

    void set_active_scene(const ResourceReference<Scene>& scene);
    [[nodiscard]] ResourceReference<Scene> get_active_scene() const { return active_scene; }

    void begin_frame(FrameContext& frame) override;
    void update(FrameContext& frame) override;
    void fixed_update(FrameContext& frame) override;
    void post_update(FrameContext& frame) override;

private:
    ResourceReference<Scene> active_scene;

    std::vector<std::unique_ptr<ecs::SystemBase>> input_systems;
    std::vector<std::unique_ptr<ecs::SystemBase>> fixed_update_systems;
    std::vector<std::unique_ptr<ecs::SystemBase>> update_systems;
    std::vector<std::unique_ptr<ecs::SystemBase>> pre_render_systems;
    //
    // std::unique_ptr<BasePlayerInputSystem> player_input_system;
    // std::unique_ptr<BaseCameraSystem> camera_system;
    // std::unique_ptr<TransformHierarchySystem> transform_system;
    // std::unique_ptr<SceneRenderingSystem> scene_rendering_system;
};
} // portal
