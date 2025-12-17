//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "composite.h"

#include <ranges>

namespace portal
{
std::optional<ResourceReference<renderer::Texture>> Composite::get_texture(const StringId& resource_id) const
{
    if (!resources.contains(ResourceType::Texture))
        return std::nullopt;

    if (!resources.at(ResourceType::Texture).contains(resource_id))
        return std::nullopt;

    return resources.at(ResourceType::Texture).at(resource_id).cast<renderer::Texture>();
}

std::optional<ResourceReference<renderer::Material>> Composite::get_material(const StringId& resource_id) const
{
    if (!resources.contains(ResourceType::Material))
        return std::nullopt;

    if (!resources.at(ResourceType::Material).contains(resource_id))
        return std::nullopt;

    return resources.at(ResourceType::Material).at(resource_id).cast<renderer::Material>();
}

std::optional<ResourceReference<MeshGeometry>> Composite::get_mesh(const StringId& resource_id) const
{
    if (!resources.contains(ResourceType::Mesh))
        return std::nullopt;

    if (!resources.at(ResourceType::Mesh).contains(resource_id))
        return std::nullopt;

    return resources.at(ResourceType::Mesh).at(resource_id).cast<MeshGeometry>();
}

std::optional<ResourceReference<Scene>> Composite::get_scene(const StringId& resource_id) const
{
    if (!resources.contains(ResourceType::Scene))
        return std::nullopt;

    if (!resources.at(ResourceType::Scene).contains(resource_id))
        return std::nullopt;

    return resources.at(ResourceType::Scene).at(resource_id).cast<Scene>();
}

auto Composite::list_scenes() const
{
    return resources.at(ResourceType::Scene) | std::views::keys;
}

void Composite::set_resource(const ResourceType type, const StringId& resource_id, const ResourceReference<Resource>& resource)
{
    resources[type][resource_id] = resource;
}
} // portal
