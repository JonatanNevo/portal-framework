//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "material_loader.h"

#include "portal/engine/project/project.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"
#include "portal/engine/resources/resource_reference.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/source/resource_source.h"

namespace portal::renderer::vulkan
{
class VulkanMaterial;
}

namespace portal::resources
{
MaterialLoader::MaterialLoader(const Project& project, ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context) :
    ResourceLoader(registry),
    context(context),
    project(project)
{
}

Reference<Resource> MaterialLoader::load(const SourceMetadata& meta, const ResourceSource& source)
{
    auto material_meta = std::get<MaterialMetadata>(meta.meta);

    auto shader = registry.immediate_load<renderer::vulkan::VulkanShader>(material_meta.shader);
    const auto hash = shader->compile_with_permutations({});
    const auto variant = shader->get_shader(hash).lock();

    renderer::MaterialProperties properties{
        .id = meta.resource_id,
        .shader = variant,
        // TODO: get this based on the loaded project / pipeline?
        .global_descriptor_sets = {STRING_ID("scene_data")},
        // TODO: determine the amount of global descriptors based on loaded project
        .set_start_index = 1,
        .frames_in_flight = project.get_settings().get_setting<size_t>("application.frames_in_flight", 3),
        .default_texture = registry.get<renderer::Texture>(renderer::Texture::MISSING_TEXTURE_ID).underlying(),
    };

    MaterialDetails details;
    if (meta.format == SourceFormat::Memory)
        details = load_details_from_memory(source);
    else
        throw std::runtime_error("Unknown material format");


    const auto material = make_reference<renderer::vulkan::VulkanMaterial>(properties, context);

    // TODO: make this generic
    material->set(STRING_ID("material_data.color_factors"), details.color_factors);
    material->set(STRING_ID("material_data.metal_rough_factors"), details.metallic_factors);

    if (details.color_texture != INVALID_STRING_ID)
    {
        auto texture_ref = registry.immediate_load<renderer::Texture>(details.color_texture);
        material->set(STRING_ID("material_data.color_texture"), texture_ref);
    }
    else
    {
        material->set(STRING_ID("material_data.color_texture"), registry.get<renderer::Texture>(renderer::Texture::WHITE_TEXTURE_ID));
    }

    if (details.metallic_texture != INVALID_STRING_ID)
    {
        auto texture_ref = registry.immediate_load<renderer::Texture>(details.metallic_texture);
        material->set(STRING_ID("material_data.metal_rough_texture"), texture_ref);
    }
    else
    {
        material->set(STRING_ID("material_data.metal_rough_texture"), registry.get<renderer::Texture>(renderer::Texture::WHITE_TEXTURE_ID));
    }

    if (details.pass_type == MaterialPass::Transparent)
        material->set_pipeline(reference_cast<renderer::vulkan::VulkanPipeline>(create_pipeline(STRING_ID("transparent_pipeline"), variant, false)));
    else
        material->set_pipeline(reference_cast<renderer::vulkan::VulkanPipeline>(create_pipeline(STRING_ID("color_pipeline"), variant, true)));

    return material;
}

void MaterialLoader::enrich_metadata(SourceMetadata& meta, const ResourceSource&)
{
    meta.dependencies.push_back(STRING_ID("engine/shaders/pbr"));
    meta.meta = MaterialMetadata{
        .shader = STRING_ID("engine/shaders/pbr")
    };
}

MaterialDetails MaterialLoader::load_details_from_memory(const ResourceSource& source)
{
    auto data = source.load();

    return data.read<MaterialDetails>();
}

Reference<renderer::Pipeline> MaterialLoader::create_pipeline(const StringId& name, const Reference<renderer::ShaderVariant>& shader, bool depth)
{
    if (name == STRING_ID("transparent_pipeline") && transparent_pipeline != nullptr)
        return transparent_pipeline;

    if (name == STRING_ID("color_pipeline") && color_pipeline != nullptr)
        return color_pipeline;

    // TODO: add pipeline cache
    renderer::PipelineProperties pipeline_properties{
        .shader = shader,
        .attachments = {
            // TODO: Find a way to extract this from current swapchain
            .attachment_images = {
                // Present Image
                {
                    .format = renderer::ImageFormat::SRGBA,
                    .blend = false
                },
                // Depth Image
                {
                    .format = renderer::ImageFormat::Depth_32Float,
                    .blend = true,
                    .blend_mode = renderer::BlendMode::Additive
                }
            },
            .blend = true,
        },
        .topology = renderer::PrimitiveTopology::Triangles,
        .depth_compare_operator = renderer::DepthCompareOperator::GreaterOrEqual,
        .backface_culling = false,
        .depth_test = depth,
        .depth_write = depth,
        .wireframe = false,
        .debug_name = name
    };
    auto pipeline = make_reference<renderer::vulkan::VulkanPipeline>(pipeline_properties, context);

    if (name == STRING_ID("color_pipeline"))
        color_pipeline = pipeline;

    if (name == STRING_ID("transparent_pipeline"))
        transparent_pipeline = pipeline;

    return pipeline;
}
} // portal
