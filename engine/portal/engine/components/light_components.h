//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <glm/vec3.hpp>

#include "register_component.h"
#include "portal/engine/renderer/image/texture.h"
#include "portal/engine/resources/resource_reference.h"

namespace portal
{
enum class LightType
{
    None,
    Directional,
    Point,
    Spot
};

struct DirectionalLightComponent
{
    glm::vec3 direction = { 0.0f, 0.0f, 0.0f };
    glm::vec3 radiance { 1.f, 1.f, 1.f};
    float intensity = 1.f;
    float light_size = 0.5f;

    // TODO: implement shadows
    // bool cast_shadows = true;
    // bool soft_shadows = true;
    // float shadow_amount = 1.f;
};

struct PointLightComponent
{
    glm::vec3 radiance { 1.f, 1.f, 1.f};
    float intensity = 1.f;
    float light_size = 0.5f;
    float min_radius = 1.f;
    float radius = 10.f;
    float falloff = 1.f;

    // TODO: implement shadows
    // bool cast_shadows = true;
    // bool soft_shadows = true;
};

struct SpotlightComponent
{
    glm::vec3 radiance { 1.f, 1.f, 1.f};
    float intensity = 1.f;
    float range = 10.f;
    float angle = 60.f;
    float angle_attenuation = 5.f;
    float falloff = 1.f;

    // TODO: implement shadows
    // bool cast_shadows = true;
    // bool soft_shadows = true;
};

struct SkylightComponent
{
    ResourceReference<renderer::Texture> radiance_map;
    ResourceReference<renderer::Texture> irradiance_map;
    float intensity = 1.f;
    float lod = 0.f;

    void archive(ArchiveObject& archive) const;
    static SkylightComponent dearchive(ArchiveObject& archive);

    void serialize(Serializer& serializer) const;
    static SkylightComponent deserialize(Deserializer& serializer);

    void post_serialization(Entity entity, ResourceRegistry& reg);
    std::vector<StringId> get_dependencies() const;
};

REGISTER_COMPONENT(DirectionalLightComponent);
REGISTER_COMPONENT(PointLightComponent);
REGISTER_COMPONENT(SpotlightComponent);
REGISTER_COMPONENT(SkylightComponent);

}
