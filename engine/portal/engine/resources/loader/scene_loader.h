//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <variant>

#include "llvm/ADT/SmallVector.h"
#include "portal/core/glm.h"
#include "portal/engine/resources/loader/loader.h"
#include "portal/engine/strings/string_id.h"

namespace portal
{
namespace scene
{
    class Node;
}

class RendererContext;
}

namespace portal::resources
{

struct TransformComponent
{
    glm::mat4 transform = glm::identity<glm::mat4>();
};

struct MeshComponent
{
    StringId mesh_id;
    std::vector<StringId> materials;
};

struct NodeDescription
{
    StringId name;
    std::vector<StringId> children{};
    std::optional<StringId> parent = std::nullopt;

    std::vector<std::variant<TransformComponent, MeshComponent>> components{};
};

struct SceneDescription
{
    std::vector<NodeDescription> nodes;
    std::vector<size_t> scene_nodes_ids;
};

class SceneLoader final : public ResourceLoader
{
public:
    SceneLoader(ResourceRegistry& registry, const RendererContext& context);
    Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;

protected:
    std::unordered_map<StringId, Reference<scene::Node>> load_scene_nodes(SceneDescription description) const;

    SceneDescription load_scene_description(const SourceMetadata& meta, const ResourceSource& source);

private:
    const RendererContext& context;
};

}
