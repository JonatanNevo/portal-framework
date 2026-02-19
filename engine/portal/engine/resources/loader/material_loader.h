//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <glm/vec4.hpp>

#include "portal/engine/resources/loader/loader.h"
#include "portal/core/strings/string_id.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"

namespace portal
{
class Project;

namespace renderer
{
    class ShaderVariant;
    class Pipeline;
}

namespace renderer::vulkan
{
    class VulkanPipeline;
}

class RendererContext;
}

namespace portal::resources
{
enum class MaterialPass: uint8_t
{
    Transparent,
    MainColor
};

// TODO: Define some generic standard material format to communicate between this and gltf loader + save to filesystem
struct MaterialDetails
{
    glm::vec3 albedo;
    float metallic = 0.f;
    float roughness = 0.5f;
    float emission = 0.f;
    float env_map_rotation = 0.f;

    MaterialPass pass_type;

    StringId color_texture = INVALID_STRING_ID;
    StringId normal_texture = INVALID_STRING_ID;
    StringId roughness_texture = INVALID_STRING_ID;
    StringId metallic_texture = INVALID_STRING_ID;

    static MaterialDetails dearchive(ArchiveObject& archive);
};

class MaterialLoader final : public ResourceLoader
{
public:
    MaterialLoader(const Project& project, ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context);

    ResourceData load(const SourceMetadata& meta, Reference<ResourceSource> source) override;
    static void enrich_metadata(SourceMetadata& meta, const ResourceSource& source);
    void save(ResourceData& resource_data)override;

protected:
    [[nodiscard]] static MaterialDetails load_details_from_memory(const ResourceSource& source);
    [[nodiscard]] static MaterialDetails load_details_from_file(const ResourceSource& source);

    Reference<renderer::Pipeline> create_pipeline(const StringId& name, uint64_t shader_hash, const Reference<renderer::ShaderVariant>& shader, bool depth);

private:
    const renderer::vulkan::VulkanContext& context;

    const Project& project;
    std::unordered_map<uint64_t, Reference<renderer::Pipeline>> pipeline_cache;
};
} // portal
