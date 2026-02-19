//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "light_components.h"

namespace portal
{
void SkylightComponent::archive(ArchiveObject& archive) const
{
    archive.add_property("radiance_map", radiance_map->get_id());
    archive.add_property("irradiance_map", irradiance_map->get_id());
    archive.add_property("intensity", intensity);
    archive.add_property("lod", lod);
}

SkylightComponent SkylightComponent::dearchive(ArchiveObject& archive)
{
    StringId radiance_map_id, irradiance_map_id;

    SkylightComponent comp;
    archive.get_property("radiance_map", radiance_map_id);
    archive.get_property("irradiance_map", irradiance_map_id);
    archive.get_property("intensity", comp.intensity);
    archive.get_property("lod", comp.lod);

    comp.irradiance_map = ResourceReference<renderer::Texture>(irradiance_map_id);
    comp.radiance_map = ResourceReference<renderer::Texture>(radiance_map_id);
    return comp;
}

void SkylightComponent::serialize(Serializer& serializer) const
{
    serializer.add_value(radiance_map->get_id());
    serializer.add_value(irradiance_map->get_id());
    serializer.add_value(intensity);
    serializer.add_value(lod);
}

SkylightComponent SkylightComponent::deserialize(Deserializer& serializer)
{
    StringId radiance_map_id, irradiance_map_id;
    SkylightComponent comp;

    serializer.get_value(radiance_map_id);
    serializer.get_value(irradiance_map_id);
    serializer.get_value(comp.intensity);
    serializer.get_value(comp.lod);

    comp.irradiance_map = ResourceReference<renderer::Texture>(irradiance_map_id);
    comp.radiance_map = ResourceReference<renderer::Texture>(radiance_map_id);
    return comp;
}

void SkylightComponent::post_serialization(Entity, ResourceRegistry& reg)
{
    radiance_map = reg.immediate_load<renderer::Texture>(radiance_map.get_resource_id());
    irradiance_map = reg.immediate_load<renderer::Texture>(irradiance_map.get_resource_id());
}

std::vector<StringId> SkylightComponent::get_dependencies() const
{
    return {radiance_map.get_resource_id(), irradiance_map.get_resource_id()};
}
}
