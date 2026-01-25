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
    glm::vec4 color_factors;
    glm::vec4 metallic_factors;
    MaterialPass pass_type;

    StringId color_texture = INVALID_STRING_ID;
    StringId metallic_texture = INVALID_STRING_ID;
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

    Reference<renderer::Pipeline> create_pipeline(const StringId& name, const Reference<renderer::ShaderVariant>& shader, bool depth);

private:
    const renderer::vulkan::VulkanContext& context;

    const Project& project;
    Reference<renderer::vulkan::VulkanPipeline> transparent_pipeline;
    Reference<renderer::vulkan::VulkanPipeline> color_pipeline;
};
} // portal
