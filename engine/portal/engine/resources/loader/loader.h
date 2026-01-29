//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file loader.h
 * @brief Abstract interface for resource loaders
 *
 * This file defines the ResourceLoader interface that all concrete loaders must implement.
 * Loaders are responsible for decoding source data (files, memory buffers) into typed
 * resource objects that can be used by the engine.
 *
 * @see LoaderFactory for the type-to-loader mapping
 * @see TextureLoader, MeshLoader, MaterialLoader for concrete implementations
 */

#pragma once
#include "portal/engine/reference.h"
#include "portal/engine/resources/database/resource_database.h"

namespace portal
{
class ArchiveObject;
class Resource;
struct SourceMetadata;
class ResourceRegistry;
}

namespace portal::resources
{
class ResourceSource;

struct ResourceData
{
    Reference<Resource> resource = nullptr;
    Reference<ResourceSource> source = nullptr;
    SourceMetadata metadata{};
    ResourceDirtyFlags dirty = ResourceDirtyBits::Clean;
};

/**
 * @class ResourceLoader
 * @brief Abstract base class for all resource loaders
 *
 * ResourceLoader defines the interface for loading typed resources from source data.
 * Each loader is specialized for a specific ResourceType (Texture, Mesh, Material, etc.)
 * and knows how to decode the relevant file formats.
 *
 * Loader Responsibilities:
 * - Read bytes from the provided ResourceSource (file, memory, etc.)
 * - Decode the source format (PNG, GLTF, OBJ, etc.) into engine data structures
 * - Create GPU resources (textures, buffers) via the RendererContext
 * - Handle dependencies by loading required sub-resources through the registry
 * - Return a Reference<Resource> that can be cast to the concrete type
 *
 * Threading Model:
 * Loaders run on job system worker threads. The load() method blocks the worker thread
 * during loading, but since it's on the job system, other workers continue processing.
 * Loaders should avoid long-running operations that can't be parallelized.
 *
 * Dependency Handling:
 * If a resource depends on other resources (e.g., materials depend on textures),
 * loaders can request them from the registry:
 * @code
 * // Inside MaterialLoader::load()
 * auto albedo_ref = registry.load<TextureResource>(meta.albedo_texture_id);
 * if (albedo_ref.is_valid()) {
 *     material->set_albedo_texture(albedo_ref.get());
 * }
 * @endcode
 *
 * @see LoaderFactory::get() for how loaders are selected by ResourceType
 * @see ResourceRegistry::load_direct() for how loaders are invoked
 * @see ResourceSource for the data reading abstraction
 */
class ResourceLoader
{
public:
    /**
     * @brief Constructor
     * @param registry Reference to the ResourceRegistry for loading dependencies
     */
    explicit ResourceLoader(ResourceRegistry& registry) : registry(registry) {}
    virtual ~ResourceLoader() = default;

    /**
     * @brief Load a resource from source data
     *
     * This method blocks the calling thread (job system worker) until the resource is
     * fully loaded and ready for use. It reads bytes from the source, decodes the format,
     * creates GPU resources if needed, and returns a Reference to the loaded resource.
     *
     * If the resource depends on other resources, the loader can request them from the
     * registry. The registry will handle scheduling those loads appropriately.
     *
     * Error Handling:
     * If loading fails (corrupt data, unsupported format, out of memory, etc.), the
     * loader should log the error and return nullptr. The registry will move the
     * resource to ResourceState::Error.
     *
     * @param meta Metadata about the resource (type, format, dependencies, etc.)
     * @param source Abstraction for reading the source bytes (file, memory, etc.)
     * @return ResourceData if successful, nullptr on error
     *
     * @note The returned resource is managed by Reference (std::shared_ptr)
     * @note Implementations should be thread-safe for reading from the source
     */
    virtual ResourceData load(const SourceMetadata& meta, Reference<ResourceSource> source) = 0;

    /**
     * @brief Saves a resource to a source.
     *
     * This method blocks the calling thread until the save operation is complete.
     *
     * This method will fetch the resource's dirty state and will update the source (if supported) based on it.
     *
     * @param resource_data The resource to save
     */
    virtual void save(ResourceData& resource_data) = 0;

    virtual void snapshot(const ResourceData&, Reference<ResourceSource>) {};
    virtual void load_snapshot(const ResourceData&, Reference<ResourceSource>) {};

protected:
    /** @brief Reference to the registry for loading dependent resources */
    ResourceRegistry& registry;
};
}
