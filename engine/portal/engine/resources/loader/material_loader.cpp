//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "material_loader.h"

#include "portal/core/buffer_stream.h"
#include "portal/engine/project/project.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/renderer/shaders/shader_types.h"
#include "portal/engine/renderer/vulkan/vulkan_material.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"
#include "portal/engine/resources/resource_reference.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/serialization/archive/json_archive.h"

namespace portal::renderer::vulkan
{
class VulkanMaterial;
}

namespace portal::resources
{
MaterialDetails MaterialDetails::dearchive(ArchiveObject& archive)
{
    MaterialDetails details;
    archive.get_property("surface_color", details.surface_color);
    archive.get_property("roughness", details.roughness);
    archive.get_property("subsurface", details.subsurface);
    archive.get_property("sheen", details.sheen);
    archive.get_property("sheen_tint", details.sheen_tint);
    archive.get_property("anistropy", details.anistropy);
    archive.get_property("specular_strength", details.specular_strength);
    archive.get_property("metallic", details.metallic);
    archive.get_property("specular_tint", details.specular_tint);
    archive.get_property("clearcoat", details.clearcoat);
    archive.get_property("clearcoat_gloss", details.clearcoat_gloss);
    archive.get_property("pass_type", details.pass_type);
    archive.get_property("color_texture", details.color_texture);
    archive.get_property("normal_texture", details.normal_texture);
    archive.get_property("metallic_roughness_texture", details.metallic_roughness_texture);
    return details;
}

MaterialLoader::MaterialLoader(const Project& project, ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context) :
    ResourceLoader(registry),
    context(context),
    project(project)
{
}

ResourceData MaterialLoader::load(const SourceMetadata& meta, Reference<ResourceSource> source)
{
    auto material_meta = std::get<MaterialMetadata>(meta.meta);

    // Load material details first so we can derive specialization constants
    MaterialDetails details;
    if (meta.format == SourceFormat::Memory)
        details = load_details_from_memory(*source);
    else if (meta.format == SourceFormat::Material)
        details = load_details_from_file(*source);
    else
        throw std::runtime_error("Unknown material format");

    const bool has_normal = details.normal_texture != INVALID_STRING_ID;
    const bool has_roughness = details.metallic_roughness_texture != INVALID_STRING_ID;

    // Build specialization constants matching the order of extern const static declarations in the shader
    std::vector<renderer::ShaderStaticConstants> spec_constants = {
        {"has_normal_texture", "bool", has_normal ? "true" : "false"},
        // Not yet in MaterialDetails
        {"has_tangent_texture", "bool", "false"},
        {"has_metallic_roughness_texture", "bool", has_roughness ? "true" : "false"},
        {"has_metalic_texture", "bool", "true"},
        {"has_roughness_texture", "bool", "true"},
    };

    auto shader = registry.immediate_load<renderer::vulkan::VulkanShader>(material_meta.shader);
    const auto hash = shader->compile_with_permutations({}, spec_constants);
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

    const auto material = make_reference<renderer::vulkan::VulkanMaterial>(properties, context);

    // TODO: make this generic
    material->set(STRING_ID("material_data.surface_color"), details.surface_color);
    material->set(STRING_ID("material_data.roughness"), details.roughness);
    material->set(STRING_ID("material_data.subsurface"), details.subsurface);
    material->set(STRING_ID("material_data.sheen"), details.sheen);
    material->set(STRING_ID("material_data.sheen_tint"), details.sheen_tint);
    material->set(STRING_ID("material_data.anistropy"), details.anistropy);
    material->set(STRING_ID("material_data.specular_strength"), details.specular_strength);
    material->set(STRING_ID("material_data.metallic"), details.metallic);
    material->set(STRING_ID("material_data.specular_tint"), details.specular_tint);
    material->set(STRING_ID("material_data.clearcoat"), details.clearcoat);
    material->set(STRING_ID("material_data.clearcoat_gloss"), details.clearcoat_gloss);


    if (details.color_texture != INVALID_STRING_ID)
    {
        auto texture_ref = registry.immediate_load<renderer::Texture>(details.color_texture);
        material->set(STRING_ID("material_data.color_texture"), texture_ref);
    }
    else
    {
        material->set(STRING_ID("material_data.color_texture"), registry.get<renderer::Texture>(renderer::Texture::WHITE_TEXTURE_ID));
    }

    // Only bind optional textures when their specialization constant is true
    if (has_normal)
    {
        auto texture_ref = registry.immediate_load<renderer::Texture>(details.normal_texture);
        material->set(STRING_ID("material_data.normal_texture"), texture_ref);
    }

    if (has_roughness)
    {
        auto texture_ref = registry.immediate_load<renderer::Texture>(details.metallic_roughness_texture);
        material->set(STRING_ID("material_data.metallic_roughness_texture"), texture_ref);
    }

    if (details.pass_type == MaterialPass::Transparent)
        material->set_pipeline(
            reference_cast<renderer::vulkan::VulkanPipeline>(
                create_pipeline(STRING_ID(fmt::format("transparent_pipeline_{}", material->get_id().string)), hash, variant, false)
            )
        );
    else
        material->set_pipeline(
            reference_cast<renderer::vulkan::VulkanPipeline>(
                create_pipeline(STRING_ID(fmt::format("color_pipeline_{}", material->get_id().string)), hash, variant, true)
            )
        );

    return {material, source, meta};
}


void MaterialLoader::enrich_metadata(SourceMetadata& meta, const ResourceSource&)
{
    meta.dependencies.push_back(STRING_ID("engine/shaders/pbr"));
    meta.meta = MaterialMetadata{
        .shader = STRING_ID("engine/shaders/pbr")
    };
}

void MaterialLoader::save(ResourceData&) {}

MaterialDetails MaterialLoader::load_details_from_memory(const ResourceSource& source)
{
    auto data = source.load();

    return data.read<MaterialDetails>();
}

MaterialDetails MaterialLoader::load_details_from_file(const ResourceSource& source)
{
    const auto data = source.load();
    BufferStreamReader reader(data);

    JsonArchive archive;
    archive.read(reader);

    return MaterialDetails::dearchive(archive);
}

Reference<renderer::Pipeline> MaterialLoader::create_pipeline(
    const StringId& name,
    const uint64_t shader_hash,
    const Reference<renderer::ShaderVariant>& shader,
    const bool depth
)
{
    const auto cache_key = name.id ^ shader_hash;
    if (const auto it = pipeline_cache.find(cache_key); it != pipeline_cache.end())
        return it->second;

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

    pipeline_cache[cache_key] = pipeline;
    return pipeline;
}
} // portal
