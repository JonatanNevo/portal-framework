//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file resource_database.h
 * @brief Filesystem abstraction and metadata management for the resource system
 *
 * This file defines the ResourceDatabase interface and SourceMetadata structures that
 * provide filesystem abstraction for resource loading. The database discovers resources
 * by scanning directories, extracts metadata, and provides ResourceSource abstractions
 * for reading file data.
 *
 * @see FolderResourceDatabase for the concrete filesystem-based implementation
 * @see ResourceRegistry for how the database integrates with loading
 */

#pragma once
#include <expected>
#include <llvm/ADT/SmallVector.h>

#include "portal/application/modules/module.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/image/image_types.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/serialization/archive.h"


namespace portal
{
namespace resources
{
    class ResourceSource;
}

struct SourceMetadata;

/**
 * @struct EmptyMeta
 * @brief Placeholder metadata for resources without format-specific metadata
 *
 * Used as the default variant value in SourceMetadata::meta when a resource type
 * doesn't require additional metadata beyond the base SourceMetadata fields.
 */
struct EmptyMeta
{
    void archive(ArchiveObject&) const {}
};

/**
 * @struct CompositeMetadata
 * @brief Metadata for composite resources containing multiple sub-resources
 *
 * Composite resources (like GLTF files) contain multiple embedded assets. This metadata
 * stores the child resources discovered during metadata enrichment. The GltfLoader uses
 * this to create separate resource entries for each texture, material, mesh, and scene
 * found in the GLTF file.
 *
 * @see GltfLoader::enrich_metadata() for how children are populated
 * @see ResourceType::Composite for composite resource handling
 */
struct CompositeMetadata
{
    /** @brief Map of child resource names to their metadata */
    std::unordered_map<std::string, SourceMetadata> children;

    /** @brief Type identifier for the composite (e.g., "gltf") */
    std::string type;

    void archive(ArchiveObject& archive) const;
    static CompositeMetadata dearchive(ArchiveObject& archive);
};

/**
 * @struct TextureMetadata
 * @brief Format-specific metadata for texture resources
 *
 * Extracted during database scanning or loader enrichment. Contains information about
 * the texture format that loaders need to properly decode and upload to GPU memory.
 */
struct TextureMetadata
{
    bool hdr;
    size_t width;
    size_t height;
    renderer::ImageFormat format;

    void archive(ArchiveObject& archive) const;
    static TextureMetadata dearchive(const ArchiveObject& archive);
};

/**
 * @struct MaterialMetadata
 * @brief Format-specific metadata for material resources
 *
 * Materials reference shaders and textures. This metadata stores the shader reference
 * that defines how the material should be rendered.
 */
struct MaterialMetadata
{
    StringId shader;

    void archive(ArchiveObject& archive) const;
    static MaterialMetadata dearchive(const ArchiveObject& archive);
};

/**
 * @struct FontMetadata
 * @brief Format-specific metadata for font resources
 */
struct FontMetadata
{
    StringId name{};
    uint16_t glyph_range_min{};
    uint16_t glyph_range_max{};

    void archive(ArchiveObject& archive) const;
    static FontMetadata dearchive(const ArchiveObject& archive);
};

/**
 * @struct SourceMetadata
 * @brief Complete metadata for a resource, used by loaders and the registry
 *
 * SourceMetadata contains everything a loader needs to load a resource:
 * - Resource identity (resource_id, type)
 * - Source location (source path, format)
 * - Dependencies (other resources this depends on)
 * - Format-specific metadata (texture dimensions, composite children, etc.)
 *
 * The database populates this during filesystem scanning and loaders can enrich it
 * with additional metadata (e.g., GltfLoader adds CompositeMetadata with children).
 *
 * Usage Example:
 * @code
 * // Database provides metadata
 * auto meta = database.find(STRING_ID("textures/albedo.png")).value();
 * // meta.type == ResourceType::Texture
 * // meta.format == SourceFormat::Image
 * // meta.source == STRING_ID("textures/albedo.png")
 * // std::get<TextureMetadata>(meta.meta) contains dimensions and format
 *
 * // Loader uses metadata to load
 * auto source = database.create_source(meta.resource_id, meta);
 * auto resource = loader.load(meta, *source);
 * @endcode
 *
 * @see ResourceDatabase::find() for querying metadata
 * @see ResourceLoader::load() for how loaders consume metadata
 * @see LoaderFactory::enrich_metadata() for metadata enrichment
 */
struct SourceMetadata
{
    // Resource Information
    StringId resource_id = INVALID_STRING_ID;
    ResourceType type = ResourceType::Unknown;
    llvm::SmallVector<StringId> dependencies{};

    // Source Information
    StringId source = INVALID_STRING_ID;
    SourceFormat format = SourceFormat::Unknown;

    // For internal use only
    StringId full_source_path = INVALID_STRING_ID;

    // Specific metadata
    /**
     * @brief Format-specific metadata variant
     *
     * Contains additional metadata specific to the resource type:
     * - TextureMetadata: HDR flag, dimensions, format
     * - CompositeMetadata: Child resources for GLTF files
     * - MaterialMetadata: Shader reference
     * - EmptyMeta: No additional metadata needed
     */
    std::variant<TextureMetadata, CompositeMetadata, MaterialMetadata, EmptyMeta, FontMetadata> meta = EmptyMeta{};

    void archive(ArchiveObject& archive) const;
    static SourceMetadata dearchive(ArchiveObject& archive);
};

/**
 * @enum DatabaseErrorBit
 * @brief Error codes for database operations (bitfield flags)
 *
 * Database operations can fail for various reasons. These error flags can be combined to
 * indicate multiple issues (e.g., MissingResource | StaleMetadata).
 */
enum class DatabaseErrorBit: uint8_t
{
    Success         = 0b00000000,
    NotFound        = 0b00000001,
    Conflict        = 0b00000010,
    MissingResource = 0b00000100,
    StaleMetadata   = 0b00001000,
    MissingMetadata = 0b00010000,
    CorruptMetadata = 0b00100000,
    DatabaseMissing = 0b01000000,


    Unspecified = std::numeric_limits<uint8_t>::max(),
};

/** @brief Combined flags type for database errors */
using DatabaseError = Flags<DatabaseErrorBit>;

namespace resources
{
    struct DatabaseEntry
    {
        StringId name;
        DatabaseEntry* parent = nullptr;
        std::unordered_map<StringId, DatabaseEntry> children;

        std::filesystem::path get_path() const;
    };
}

/**
 * @class ResourceDatabase
 * @brief Abstract interface for resource metadata storage and file access
 *
 * The ResourceDatabase provides filesystem abstraction for the resource system. It discovers
 * resources by scanning directories, extracts and caches metadata, and creates ResourceSource
 * abstractions for reading file data.
 *
 * Responsibilities:
 * - Discovery: Scan filesystem directories to find resource files
 * - Metadata: Extract and cache SourceMetadata for each resource
 * - Persistence: Save/load metadata to avoid re-scanning on startup
 * - Source Creation: Provide ResourceSource objects for reading file bytes
 *
 * @see FolderResourceDatabase for the concrete filesystem implementation
 * @see ResourceSource for the file reading abstraction
 * @see SourceMetadata for what metadata is stored
 */
class ResourceDatabase
{
public:
    virtual ~ResourceDatabase() = default;

    /**
     * @brief Find metadata for a resource by its ID
     *
     * @param resource_id StringId of the resource to find
     * @return SourceMetadata if found, or DatabaseError explaining why lookup failed
     */
    virtual std::expected<SourceMetadata, DatabaseError> find(StringId resource_id) = 0;

    /**
     * @brief Add a new resource to the database
     *
     * @param resource_id StringId for the new resource
     * @param meta Complete metadata for the resource
     * @return Success or error flags indicating why the operation failed
     */
    virtual DatabaseError add(StringId resource_id, SourceMetadata meta) = 0;

    /**
     * @brief Remove a resource from the database
     *
     * @param resource_id StringId of the resource to remove
     * @return Success if removed, NotFound if resource didn't exist
     */
    virtual DatabaseError remove(StringId resource_id) = 0;

    /**
     * @brief Create a ResourceSource for reading resource data
     *
     * @param resource_id StringId of the resource
     * @param meta Metadata describing the resource
     * @return Unique pointer to a ResourceSource for reading bytes
     */
    virtual Reference<resources::ResourceSource> create_source(StringId resource_id, SourceMetadata meta) = 0;

    [[nodiscard]] virtual resources::DatabaseEntry& get_structure() const = 0;

    [[nodiscard]] virtual StringId get_name() const = 0;
};
} // portal
