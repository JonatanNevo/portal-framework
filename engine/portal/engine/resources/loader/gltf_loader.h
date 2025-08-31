//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <fastgltf/core.hpp>

#include "portal/engine/resources/loader/loader.h"
#include "portal/engine/resources/resources/material.h"
#include "portal/engine/scene/scene.h"

namespace portal::renderer::vulkan
{
class GpuContext;
}

namespace portal::resources
{
class LoaderFactory;

class GltfLoader final : public ResourceLoader
{
public:
    GltfLoader(ResourceRegistry* registry, const std::shared_ptr<renderer::vulkan::GpuContext>& context);

    bool load(std::shared_ptr<ResourceSource> source) const override;
    void load_default(Ref<Resource>& resource) const override;
protected:
    static fastgltf::Asset load_from_source(const std::shared_ptr<ResourceSource>& source, fastgltf::GltfDataGetter& data);

    void load_texture(const fastgltf::Asset& asset, const fastgltf::Texture& texture) const;
    void load_material(size_t index, const fastgltf::Asset& asset, const fastgltf::Material& material) const;
    void load_mesh(size_t index, const fastgltf::Asset& asset, const fastgltf::Mesh& mesh) const;
    std::vector<Ref<Scene>> load_scenes(const fastgltf::Asset& asset) const;

    Ref<renderer::Pipeline> create_pipeline(const StringId& name, const Ref<renderer::Shader>& shader, bool depth) const;

private:
    std::shared_ptr<renderer::vulkan::GpuContext> gpu_context;
};

} // portal
