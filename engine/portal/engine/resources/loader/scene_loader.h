//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <variant>

#include "llvm/ADT/SmallVector.h"
#include "portal/core/glm.h"
#include "portal/engine/resources/loader/loader.h"
#include "portal/core/strings/string_id.h"
#include "portal/engine/ecs/registry.h"

namespace portal
{
class RendererContext;
}

namespace portal::resources
{
struct TransformSceneComponent
{
    glm::mat4 transform = glm::identity<glm::mat4>();
};

struct MeshSceneComponent
{
    StringId mesh_id;
    std::vector<StringId> materials;
};

struct NodeDescription
{
    StringId name;
    std::vector<StringId> children{};
    std::optional<StringId> parent = std::nullopt;

    std::vector<std::variant<TransformSceneComponent, MeshSceneComponent>> components{};
};

struct SceneDescription
{
    std::vector<NodeDescription> nodes;
    std::vector<size_t> scene_nodes_ids;
};

class SceneLoader final : public ResourceLoader
{
public:
    explicit SceneLoader(ResourceRegistry& registry);
    Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;

protected:
    void load_scene_nodes(Entity scene_entity, ecs::Registry& ecs_registry, SceneDescription description) const;

    static SceneDescription load_scene_description(const SourceMetadata& meta, const ResourceSource& source);
};
}
