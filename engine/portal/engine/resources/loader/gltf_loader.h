//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file gltf_loader.h
 * @brief GLTF composite resource loader
 *
 * This file defines the GltfLoader class that handles loading GLTF files, which are
 * composite resources containing multiple sub-resources (textures, materials, meshes,
 * scenes) within a single file.
 *
 */

#pragma once
#include <filesystem>

#include "portal/core/jobs/job.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/resources/loader/loader.h"
#include "portal/engine/scene/scene.h"

namespace fastgltf
{
struct Mesh;
struct Material;
struct Texture;
class Asset;
class GltfDataGetter;
}

namespace portal
{
class RendererContext;
}

namespace portal::renderer
{
namespace vulkan
{
    class VulkanPipeline;
}

class ShaderVariant;
class Pipeline;
class Shader;
}


namespace portal::resources
{
class LoaderFactory;

/**
 * @class GltfLoader
 * @brief Loader for GLTF composite resources
 *
 * The GltfLoader handles loading GLTF (.gltf, .glb) files, which are composite resources
 * that contain multiple embedded assets within a single file. A GLTF file can contain:
 * - Multiple textures (PNG, JPEG embedded or referenced)
 * - Multiple materials (PBR material definitions)
 * - Multiple meshes (vertex data, primitives)
 * - Multiple scenes (entity hierarchies with transforms)
 *
 * Composite Resource Pattern:
 *
 * GLTF loading happens in two phases:
 *
 * 1. **Metadata Enrichment**: The database calls enrich_metadata() during filesystem
 *    scanning to discover child resources. This parses the GLTF file and creates
 *    SourceMetadata entries for each texture, material, mesh, and scene found.
 *    These are stored in CompositeMetadata::children.
 *
 * 2. **Resource Loading**: When the registry loads the GLTF, load() is called.
 *    It iterates through the children and loads each one by dispatching jobs
 *    through the registry. The main composite resource tracks these child references.
 *
 * Usage Example:
 * @code
 * // Loading a GLTF creates a composite with child resources
 * auto gltf_ref = registry.load<Composite>(STRING_ID("models/character.gltf"));
 *
 * // Wait for loading to complete
 * while (!gltf_ref.is_valid()) {
 *     // Show loading screen
 * }
 *
 * // Access child resources through the composite
 * auto& composite = gltf_ref.get();
 * auto mesh_ref = composite.get_mesh("character_body");
 * auto material_ref = composite.get_material("skin_material");
 * auto scene_ref = composite.get_scene("default_scene");
 * @endcode
 *
 * Dependencies:
 * - Uses fastgltf library for GLTF parsing (https://github.com/spnda/fastgltf)
 * - Requires RendererContext for creating GPU resources
 *
 * @see enrich_metadata() for the metadata discovery phase
 * @see load() for the loading phase
 * @see CompositeMetadata for how children are stored
 * @see ResourceType::Composite for the composite resource type
 */
class GltfLoader final : public ResourceLoader
{
public:
    /**
     * @brief Constructor
     * @param registry Reference to ResourceRegistry for loading child resources
     * @param context Reference to VulkanContext
     */
    GltfLoader(ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context);

    /**
     * @brief Load a GLTF composite resource and all its children
     *
     * @param meta Metadata with CompositeMetadata::children populated by enrich_metadata()
     * @param source ResourceSource for reading the GLTF file bytes
     * @return Reference to Composite resource, or nullptr on error
     *
     * @note This method blocks until the GLTF is parsed and child jobs are dispatched
     * @note Child resources load asynchronously on the job system
     */
    ResourceData load(const SourceMetadata& meta, Reference<ResourceSource> source) override;

    /**
     * @brief Enrich metadata by discovering child resources in a GLTF file
     *
     * This static method is called during database filesystem scanning. It parses
     * the GLTF file to discover all embedded assets and creates SourceMetadata
     * entries for each child resource. These are stored in CompositeMetadata::children.
     *
     * Discovered children include:
     * - Textures: Each image in the GLTF creates a texture child
     * - Materials: Each material creates a material child with texture dependencies
     * - Meshes: Each mesh creates a mesh child
     * - Scenes: Each scene creates a scene child with mesh/material dependencies
     *
     * @param meta SourceMetadata to enrich (will be updated with CompositeMetadata)
     * @param source ResourceSource for reading the GLTF file
     *
     * @note This is called during database scanning, before any loading occurs
     * @note The metadata is persisted to .portal-db to avoid re-parsing on startup
     */
    static void enrich_metadata(SourceMetadata& meta, const ResourceSource& source);

    void save(const ResourceData& resource_data) override;

protected:
    static fastgltf::Asset load_asset(const SourceMetadata& meta, fastgltf::GltfDataGetter& data);

    static std::pair<SourceMetadata, Reference<ResourceSource>> find_image_source(
        const std::filesystem::path& base_name,
        const std::filesystem::path& base_path,
        const fastgltf::Asset& asset,
        const fastgltf::Texture& texture
    );

    Job<> load_texture(SourceMetadata texture_meta, const fastgltf::Asset& asset, const fastgltf::Texture& texture) const;
    Job<> load_material(SourceMetadata material_meta, const fastgltf::Asset& asset, const fastgltf::Material& material) const;
    Job<> load_mesh(SourceMetadata mesh_meta, const fastgltf::Asset& asset, const fastgltf::Mesh& mesh) const;
    void load_scenes(SourceMetadata meta, const fastgltf::Asset& asset) const;

protected:
    const renderer::vulkan::VulkanContext& context;

    Reference<renderer::vulkan::VulkanPipeline> g_transparent_pipeline;
    Reference<renderer::vulkan::VulkanPipeline> g_color_pipeline;
};
} // portal
