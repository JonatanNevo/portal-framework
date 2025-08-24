//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "material.h"

namespace portal {
Ref<Pipeline> Material::get_pipeline() const
{
    PORTAL_ASSERT(pipeline.is_valid(), "Pipeline is not valid");
    return pipeline.lock();
}

const std::vector<vk::raii::DescriptorSet>& Material::get_descriptor_sets() const
{
    return descriptor_sets;
}

void Material::copy_from(const Ref<Resource> other)
{
    auto other_material = other.as<Material>();

    pass_type = other_material->pass_type;
    consts = other_material->consts;
    material_data = other_material->material_data;
    std::swap(descriptor_sets, other_material->descriptor_sets);
    std::swap(descriptor_set_layouts, other_material->descriptor_set_layouts);

    pipeline = other_material->pipeline;
    color_texture = other_material->color_texture;

    metallic_roughness_texture = other_material->metallic_roughness_texture;
    shader = other_material->shader;
}
} // portal