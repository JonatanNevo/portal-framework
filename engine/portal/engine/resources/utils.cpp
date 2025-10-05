//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "utils.h"

#include "portal/engine/resources/resources/mesh.h"
#include "portal/engine/scene/scene.h"
#include "portal/engine/renderer/shaders/shader.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/renderer/image/texture.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"

namespace portal::resources::utils
{

static auto logger = Log::get_logger("Resources");

ResourceType to_resource_type(const Ref<Resource>& resource)
{
    if (!resource)
        return ResourceType::Unknown;

    if (resource.as<renderer::Texture>())
        return ResourceType::Texture;

    if (resource.as<renderer::Material>())
        return ResourceType::Material;

    if (resource.as<renderer::Shader>())
        return ResourceType::Shader;

    if (resource.as<Mesh>())
        return ResourceType::Mesh;

    if (resource.as<Scene>())
        return ResourceType::Scene;

    return ResourceType::Unknown;
}

Ref<Resource> create_resource(const StringId& id, ResourceType type)
{
    switch (type)
    {
    case ResourceType::Material:
        return Ref<renderer::vulkan::VulkanMaterial>::create();
    case ResourceType::Texture:
        return Ref<renderer::vulkan::VulkanTexture>::create(id);
    case ResourceType::Shader:
        return Ref<renderer::vulkan::VulkanShader>::create(id);
    case ResourceType::Mesh:
        return Ref<Mesh>::create(id);
    case ResourceType::Composite:
        return Ref<Scene>::create(id); // TODO: should this return scene?
    case ResourceType::Scene:
        return Ref<Scene>::create(id);
    case ResourceType::Unknown:
        break;

    }
    LOGGER_ERROR("Failed to create resource of type {} with id {}. Returning empty resource.", type, id);
    return Ref<Resource>::create(id);
}
}
