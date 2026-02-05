//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scene_loader.h"

#include "portal/core/buffer_stream.h"
#include "portal/core/variant.h"
#include "portal/engine/components/mesh.h"
#include "portal/engine/components/relationship.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/resources/mesh_geometry.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/scene/scene.h"
#include "portal/serialization/serialize.h"
#include "portal/serialization/archive/json_archive.h"
#include "portal/serialization/serialize/binary_serialization.h"

namespace portal::resources
{
SceneLoader::SceneLoader(ResourceRegistry& registry) : ResourceLoader(registry)
{}

ResourceData SceneLoader::load(const SourceMetadata& meta, const Reference<ResourceSource> source)
{
    const auto scene = make_reference<Scene>(meta.resource_id, registry.get_ecs_registry());

    if (meta.format == SourceFormat::Scene)
    {
        load_portal_scene(scene, *source);
    }
    else if (meta.format == SourceFormat::BinaryScene)
    {
        load_binary_portal_scene(scene, *source);
    }
    else
        PORTAL_ASSERT(false, "Unsupported scene format: {}", meta.format);

    return {scene, source, meta};
}

void SceneLoader::save(ResourceData& resource_data)
{
    const auto scene = reference_cast<Scene>(resource_data.resource);
    auto& raw_registry = scene->get_registry().get_raw_registry();
    const auto dirty = resource_data.dirty;

    if (dirty & ResourceDirtyBits::DataChange | ResourceDirtyBits::StateChange)
    {
        JsonArchive archive;
        archive_scene(scene, archive);
        auto out_stream = resource_data.source->ostream();
        archive.dump(*out_stream);

        for (auto descendant : scene->get_scene_entity().descendants())
        {
            std::vector<StringId> dependencies;
            for (auto&& [type_id, storage] : raw_registry.storage())
            {
                auto type = entt::resolve(storage.info());
                if (type)
                {
                    auto result = type.invoke(
                        static_cast<entt::id_type>(STRING_ID("find_dependencies").id),
                        {},
                        entt::forward_as_meta(descendant)
                    );
                    if (result)
                    {
                        auto result_vec = result.cast<std::vector<StringId>>();
                        dependencies.insert(
                            dependencies.end(),
                            result_vec.begin(),
                            result_vec.end()
                        );
                    }
                }
            }

            resource_data.metadata.dependencies.insert(resource_data.metadata.dependencies.end(), dependencies.begin(), dependencies.end());
        }

        std::unordered_set unique_elements(resource_data.metadata.dependencies.begin(), resource_data.metadata.dependencies.end());
        resource_data.metadata.dependencies = std::ranges::to<llvm::SmallVector<StringId>>(unique_elements);

        // TODO: This should be in the resource source class
        JsonArchive meta_archive;
        resource_data.metadata.archive(meta_archive);
        const auto metadata_path = fmt::format("{}.pmeta", resource_data.metadata.full_source_path.string);
        meta_archive.dump(std::filesystem::path(metadata_path));
    }
}

void SceneLoader::load_snapshot(const ResourceData& resource_data, const Reference<ResourceSource> snapshot_source)
{
    const auto istream = snapshot_source->istream();
    BinaryDeserializer deserializer(*istream);

    deserialize_scene(reference_cast<Scene>(resource_data.resource), deserializer);
}

void SceneLoader::snapshot(const ResourceData& resource_data, const Reference<ResourceSource> snapshot_source)
{
    const auto ostream = snapshot_source->ostream();
    BinarySerializer serializer(*ostream);

    serialize_scene(reference_cast<Scene>(resource_data.resource), serializer);
}

void SceneLoader::archive_scene(const Reference<Scene>& scene, ArchiveObject& archive)
{
    auto& ecs_registry = scene->get_registry();
    auto& raw_registry = scene->get_registry().get_raw_registry();

    archive.add_property("name", scene->get_id());

    std::vector<ArchiveObject> nodes;
    for (auto descendant : scene->get_scene_entity().descendants())
    {
        auto& object = nodes.emplace_back(ArchiveObject{});
        if (descendant.has_component<NameComponent>())
        {
            auto& [name, icon] = descendant.get_component<NameComponent>();
            object.add_property("name", name);
            object.add_property("icon", icon);
        }
        else
        {
            object.add_property("name", STRING_ID("Unnamed"));
            object.add_property("icon", ICON_FA_CUBE);
        }

        for (auto&& [type_id, storage] : raw_registry.storage())
        {
            auto type = entt::resolve(storage.info());
            if (type)
            {
                auto result = type.invoke(
                    static_cast<entt::id_type>(STRING_ID("archive").id),
                    {},
                    entt::forward_as_meta(descendant),
                    entt::forward_as_meta(object),
                    entt::forward_as_meta(ecs_registry)
                );

                if (!result)
                {
                    LOG_WARN("Failed to invoke archive for type: {}", type.name());
                }
            }
        }
    }
    archive.add_property("nodes", nodes);
}

void SceneLoader::dearchive_scene(const Reference<Scene>& scene, ArchiveObject& archive) const
{
    auto& ecs_registry = scene->get_registry();

    std::vector<ArchiveObject> nodes;
    archive.get_property("nodes", nodes);

    // Create entities and deserialize components in one pass
    // This works because descendants() serializes parents before children,
    // so when adding a child to its parent, the parent already exists
    for (auto& object : nodes)
    {
        StringId name;
        std::string icon;
        object.get_property("name", name);
        object.get_property("icon", icon);

        auto entity = ecs_registry.create_entity();
        entity.add_component<NameComponent>(name, icon);

        for (const auto& [comp_name, _] : object)
        {
            auto type = entt::resolve(static_cast<entt::id_type>(STRING_ID(comp_name).id));
            if (type)
            {
                const auto result = type.invoke(
                    static_cast<entt::id_type>(STRING_ID("dearchive").id),
                    {},
                    entt::forward_as_meta(entity),
                    entt::forward_as_meta(object),
                    entt::forward_as_meta(ecs_registry)
                );

                if (!result)
                {
                    LOG_WARN("Failed to invoke dearchive for type: {}", type.name());
                }
            }
        }
    }

    // Post-Serialization pass
    for (auto& object : nodes)
    {
        StringId name;
        object.get_property("name", name);
        auto entity = ecs_registry.find_by_name(name);
        PORTAL_ASSERT(entity.has_value(), "Failed to find entity with name: {}", name);

        for (const auto& [comp_name, _] : object)
        {
            auto type = entt::resolve(static_cast<entt::id_type>(STRING_ID(comp_name).id));
            type.invoke(
                static_cast<entt::id_type>(STRING_ID("post_serialization").id),
                {},
                entt::forward_as_meta(*entity),
                entt::forward_as_meta(registry)
            );
        }
    }
}

void SceneLoader::serialize_scene(const Reference<Scene>& scene, Serializer& serializer)
{
    auto& ecs_registry = scene->get_registry();
    auto& raw_registry = scene->get_registry().get_raw_registry();

    serializer.add_value(scene->get_id());
    serializer.add_value(scene->get_scene_entity().descendants_count());
    for (auto descendant : scene->get_scene_entity().descendants())
    {
        if (descendant.has_component<NameComponent>())
        {
            auto& [name, icon] = descendant.get_component<NameComponent>();
            serializer.add_value(name);
            serializer.add_value(icon);
        }
        else
        {
            serializer.add_value(STRING_ID("Unnamed"));
            serializer.add_value(std::string{ICON_FA_CUBE});
        }

        auto num_comps = serializer.reserve<size_t>();
        size_t comp_count = 0;
        for (auto&& [type_id, storage] : raw_registry.storage())
        {
            auto type = entt::resolve(storage.info());
            if (type)
            {
                const auto result = type.invoke(
                    static_cast<entt::id_type>(STRING_ID("serialize").id),
                    {},
                    entt::forward_as_meta(descendant),
                    entt::forward_as_meta(serializer),
                    entt::forward_as_meta(ecs_registry)
                );

                if (!result)
                {
                    LOG_WARN("Failed to invoke serialize for type: {}", type.name());
                }
                else
                {
                    auto result_has_comp = result.cast<bool>();
                    if (result_has_comp)
                        ++comp_count;
                }
            }
        }
        num_comps.write(comp_count);
    }
}

void SceneLoader::deserialize_scene(const Reference<Scene>& scene, Deserializer& deserializer) const
{
    auto& ecs_registry = scene->get_registry();

    StringId name;
    size_t node_count;
    deserializer.get_value(name);
    deserializer.get_value(node_count);

    for (size_t i = 0; i < node_count; ++i)
    {
        StringId entity_name;
        std::string icon;
        deserializer.get_value(entity_name);
        deserializer.get_value(icon);

        auto entity = ecs_registry.find_or_create(entity_name);
        entity.get_component<NameComponent>().icon = icon;

        size_t component_count;
        deserializer.get_value(component_count);

        for (size_t j = 0; j < component_count; ++j)
        {
            StringId component_type;
            deserializer.get_value(component_type);

            auto type = entt::resolve(static_cast<entt::id_type>(component_type.id));
            if (type)
            {
                const auto result = type.invoke(
                    static_cast<entt::id_type>(STRING_ID("deserialize").id),
                    {},
                    entt::forward_as_meta(entity),
                    entt::forward_as_meta(deserializer),
                    entt::forward_as_meta(ecs_registry)
                );

                if (!result)
                {
                    LOG_WARN("Failed to invoke deserialize for type: {}", type.name());
                }
            }
        }
    }

    // Post-Serialization pass
    for (auto entity : scene->get_scene_entity().descendants())
    {
        for (auto&& [type_id, storage] : ecs_registry.get_raw_registry().storage())
        {
            auto type = entt::resolve(storage.info());
            if (type)
            {
                type.invoke(
                    static_cast<entt::id_type>(STRING_ID("post_serialization").id),
                    {},
                    entt::forward_as_meta(entity),
                    entt::forward_as_meta(registry)
                );
            }
        }
    }
}


void SceneLoader::load_portal_scene(const Reference<Scene>& scene, const ResourceSource& source) const
{
    const auto stream = source.istream();
    JsonArchive archive;
    archive.read(*stream);

    dearchive_scene(scene, archive);
}

void SceneLoader::load_binary_portal_scene(const Reference<Scene>& scene, const ResourceSource& source) const
{
    const auto stream = source.istream();
    BinaryDeserializer deserializer(*stream);

    deserialize_scene(scene, deserializer);
}
}
