//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/resources/resources/texture.h"

namespace portal
{
namespace renderer {
    class Pipeline;
}

namespace resources
{
    class GltfLoader;


    struct MaterialConsts
    {
        glm::vec4 color_factors;
        glm::vec4 metal_rough_factors;
        //padding, we need it anyway for uniform buffers
        glm::vec4 extra[14];
    };

    enum class MaterialPass: uint8_t
    {
        MainColor,
        Transparent,
        Other
    };
}

class Material final : public Resource
{
public:
    explicit Material(const StringId& id): Resource(id) {}
    Ref<renderer::Pipeline> get_pipeline() const;
    const std::vector<vk::raii::DescriptorSet>& get_descriptor_sets() const;

    void copy_from(Ref<Resource> other) override;

public:
    friend class resources::GltfLoader;

    resources::MaterialPass pass_type = resources::MaterialPass::Other;

    std::vector<renderer::vulkan::AllocatedBuffer> material_data;
    std::vector<vk::raii::DescriptorSet> descriptor_sets;
    WeakRef<renderer::Pipeline> pipeline;

    resources::MaterialConsts consts;
    WeakRef<Texture> color_texture;
    WeakRef<Texture> metallic_roughness_texture;
};

} // portal