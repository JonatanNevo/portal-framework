//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resource_types.h"
#include "portal/core/reference.h"
#include "portal/engine/renderer/scene/scene_node.h"
#include "portal/engine/renderer/shaders/shader_cache.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/resources/resources/material.h"
#include "portal/engine/resources/resources/mesh.h"
#include "portal/engine/scene/scene.h"

namespace portal::resources::utils
{
template<class T>
ResourceType to_resource_type()
{
    if constexpr (std::is_same_v<T, Texture>)
        return ResourceType::Texture;
    else if constexpr (std::is_same_v<T, Material>)
        return ResourceType::Material;
    else if constexpr (std::is_same_v<T, renderer::ShaderCache>)
        return ResourceType::Shader;
    else if constexpr (std::is_same_v<T, Mesh>)
        return ResourceType::Mesh;
    else if constexpr (std::is_same_v<T, Scene>)
        return ResourceType::Scene;
    else if constexpr (std::is_same_v<T, Resource>)
        return ResourceType::Unknown;
}

ResourceType to_resource_type(const Ref<Resource>& resource);

Ref<Resource> create_resource(const StringId& id, ResourceType type);
}
