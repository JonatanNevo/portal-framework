//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resource_types.h"
#include "portal/core/reference.h"
#include "portal/engine/renderer/image/texture.h"
#include "portal/engine/renderer/shaders/shader.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/resources/resources/mesh.h"
#include "portal/engine/scene/scene.h"


namespace portal::resources::utils
{
template<class T>
ResourceType to_resource_type()
{
    if constexpr (std::is_same_v<T, renderer::Texture> || std::is_same_v<T, renderer::vulkan::VulkanTexture>)
        return ResourceType::Texture;
    else if constexpr (std::is_same_v<T, renderer::Material> || std::is_same_v<T, renderer::vulkan::VulkanMaterial>)
        return ResourceType::Material;
    else if constexpr (std::is_same_v<T, renderer::Shader> || std::is_same_v<T, renderer::vulkan::VulkanShader> )
        return ResourceType::Shader;
    else if constexpr (std::is_same_v<T, Mesh>)
        return ResourceType::Mesh;
    else if constexpr (std::is_same_v<T, Scene>)
        return ResourceType::Scene;
    else
        return ResourceType::Unknown;
}

ResourceType to_resource_type(const Ref<Resource>& resource);

Ref<Resource> create_resource(const StringId& id, ResourceType type);
}
