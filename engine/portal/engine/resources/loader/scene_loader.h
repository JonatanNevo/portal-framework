//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/resources/loader/loader.h"
#include "portal/engine/ecs/registry.h"

namespace portal
{

class Serializer;
class Deserializer;

class RendererContext;
}

namespace portal::resources
{

class SceneLoader final : public ResourceLoader
{
public:
    explicit SceneLoader(ResourceRegistry& registry);

    ResourceData load(const SourceMetadata& meta, Reference<ResourceSource> source) override;
    void save(ResourceData& resource_data) override;

    void load_snapshot(const ResourceData& resource_data, Reference<ResourceSource> snapshot_source) override;
    void snapshot(const ResourceData& resource_data, Reference<ResourceSource> snapshot_source) override;


protected:
    static void archive_scene(const Reference<Scene>& scene, ArchiveObject& archive);
    static void serialize_scene(const Reference<Scene>& scene, Serializer& serializer);

    void dearchive_scene(const Reference<Scene>& scene, ArchiveObject& archive) const;
    void deserialize_scene(const Reference<Scene>& scene, Deserializer& deserializer) const;

    void load_portal_scene(const Reference<Scene>& scene, const ResourceSource& source) const;
    void load_binary_portal_scene(const Reference<Scene>& scene, const ResourceSource& source) const;
};
}
