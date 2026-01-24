//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/engine/entry_point.h"

#include "portal/engine/engine.h"
#include "portal/engine/components/base.h"
#include "portal/engine/components/base_camera_controller.h"
#include "portal/engine/components/camera.h"
#include "portal/engine/components/relationship.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/ecs/registry.h"
#include "portal/engine/resources/resources/composite.h"
#include "portal/engine/resources/source/file_source.h"
#include "portal/engine/resources/source/memory_source.h"
#include "portal/serialization/archive/json_archive.h"

using namespace portal;

std::unique_ptr<Application> portal::create_engine_application(Reference<Project>&& project, int, char**)
{
    const ApplicationProperties prop = from_project(*project);
    auto engine = std::make_unique<Engine>(project, prop);

    // TODO: Should not be here
    auto& engine_context = engine->get_engine_context();
    [[maybe_unused]] auto composite = engine_context.get_resource_registry().immediate_load<Composite>(STRING_ID("game/ABeautifulGame"));
    auto scene = engine_context.get_resource_registry().get<Scene>(STRING_ID("game/gltf-Scene-Scene"));
    PORTAL_ASSERT(scene.get_state() == ResourceState::Loaded, "Failed to load scene");

    // Serialize camera as well
    auto camera = scene->get_registry().create_entity(STRING_ID("Camera"));
    camera.add_component<PlayerTag>();
    camera.add_component<InputComponent>();
    camera.add_component<TransformComponent>();
    camera.add_component<RelationshipComponent>();
    camera.set_parent(scene->get_scene_entity());

    auto& controller = camera.add_component<BaseCameraController>();
    auto& camera_comp = camera.add_component<CameraComponent>();
    camera.add_component<MainCameraTag>();

    auto& transform = camera.get_component<TransformComponent>();
    transform.set_translation(glm::vec3(-0.51f, 0.4f, 0.74f));

    camera_comp.calculate_view(transform.get_translation(), controller.forward_direction);

    auto& resource_registry = engine_context.get_resource_registry();

    auto source = make_reference<resources::FileSource>("C:\\Users\\thejo\\OneDrive\\Documents\\PortalEngine\\scene.json");

    resources::ResourceData resource_data
    {
        .resource = scene.underlying(),
        .source = source,
        .metadata = {
            .resource_id = STRING_ID("game/gltf-Scene-Scene"),
            .type = ResourceType::Scene,
            .format = SourceFormat::Memory
        },
        .dirty = ResourceDirtyBits::DataChange
    };

    resource_registry.save_resource(resource_data);

    // auto& reg = scene->get_registry().get_raw_registry();
    // std::stringstream ss;
    // JsonArchive archive;
    //
    // std::vector<ArchiveObject> objects;
    // for (auto& entity : reg.view<entt::entity>())
    // {
    //     auto& object = objects.emplace_back(ArchiveObject{});
    //     object.add_property("id", static_cast<entt::id_type>(entity));
    //     for (auto&& [type_id, storage] : reg.storage())
    //     {
    //         auto type = entt::resolve(storage.info());
    //         Entity ecs_entity{entity, reg};
    //         type.invoke(
    //             STRING_ID("archive").id,
    //             {},
    //             entt::forward_as_meta(ecs_entity),
    //             entt::forward_as_meta(object),
    //             entt::forward_as_meta(scene->get_registry())
    //         );
    //     }
    // }
    // archive.add_property("entities", objects);
    //
    // archive.dump(ss);
    // auto string = ss.str();
    //
    // JsonArchive dearchive;
    // dearchive.read(ss);
    //
    // entt::registry deserialized_reg;
    // std::vector<ArchiveObject> deserialized_objects;
    // dearchive.get_property("entities", deserialized_objects);
    // for (auto& object : deserialized_objects)
    // {
    //     entt::id_type entity_id;
    //     object.get_property("id", entity_id);
    //
    //     auto entt_entity = deserialized_reg.create(static_cast<entt::registry::entity_type>(entity_id));
    //     Entity entity {entt_entity, deserialized_reg};
    //     for (const auto& comp_name : object | std::views::keys)
    //     {
    //         auto type = entt::resolve(STRING_ID(comp_name).id);
    //         type.invoke(
    //             STRING_ID("dearchive").id,
    //             {},
    //             entt::forward_as_meta(entity),
    //             entt::forward_as_meta(object),
    //             entt::forward_as_meta(scene->get_registry())
    //         );
    //     }
    // }
    //
    // LOG_INFO(string);
    return engine;
}
