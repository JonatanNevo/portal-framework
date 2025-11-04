//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <fastgltf/core.hpp>

#include "portal/core/jobs/job.h"
#include "portal/engine/resources/loader/loader.h"
#include "portal/engine/scene/scene.h"

namespace portal
{
class RendererContext;
}

namespace portal::renderer
{

class ShaderVariant;
class Pipeline;
class Shader;
}


namespace portal::resources
{
class LoaderFactory;

class GltfLoader final : public ResourceLoader
{
public:
    GltfLoader(ResourceRegistry& registry, const RendererContext& context);

    Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;

protected:
    static fastgltf::Asset load_asset(const SourceMetadata& meta, fastgltf::GltfDataGetter& data);

    Job<> load_texture(const fastgltf::Asset& asset, const fastgltf::Texture& texture) const;
    void load_material(size_t index, const fastgltf::Asset& asset, const fastgltf::Material& material) const;
    void load_mesh(size_t index, const fastgltf::Asset& asset, const fastgltf::Mesh& mesh) const;
    std::vector<Reference<Scene>> load_scenes(const fastgltf::Asset& asset) const;

    Reference<renderer::Pipeline> create_pipeline(const StringId& name, const Reference<renderer::ShaderVariant>& shader, bool depth) const;

protected:
    const RendererContext& context;
};

} // portal
