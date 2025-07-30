//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "utils.h"

namespace portal::resources::utils
{

static auto logger = Log::get_logger("Resources");

ResourceType to_resource_type(const Ref<Resource>& resource)
{
    if (!resource)
        return ResourceType::Unknown;

    if (resource.as<Texture>())
        return ResourceType::Texture;

    return ResourceType::Unknown;
}

Ref<Resource> create_resource(const StringId& id, ResourceType type)
{
    switch (type)
    {
    case ResourceType::Material:
        break;
    case ResourceType::Texture:
        return Ref<Texture>::create(id);
    case ResourceType::Shader:
        break;
    case ResourceType::Mesh:
        break;
    case ResourceType::Composite:
        break;
    case ResourceType::Unknown:
        break;
    }
    LOGGER_ERROR("Failed to create resource of type {} with id {}. Returning empty resource.", type, id);
    return Ref<Resource>::create(id);
}
}
