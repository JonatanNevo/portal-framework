//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file resource_types.h
 * @brief Core type definitions for the Portal Framework resource system
 *
 * This file defines the fundamental enums and utility functions used throughout the asynchronous
 * resource loading system. The resource system manages user assets (textures, meshes, materials, etc.)
 * through handle-based references with job system integration for non-blocking loading.
 *
 * The three primary enums defined here form the foundation of resource management:
 * - ResourceState: Tracks the lifecycle state of a resource reference
 * - ResourceType: Categorizes resources by their semantic type
 * - SourceFormat: Identifies the file format of resource source data
 *
 * @see ResourceRegistry for the central resource manager
 * @see ResourceReference for the smart handle type that uses these states
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <portal/core/common.h>
#include <portal/core/debug/assert.h>

#include <spdlog/spdlog.h>

namespace portal
{
/**
 * @enum ResourceState
 * @brief Represents the current state of a resource reference in its loading lifecycle
 *
 * The resource system uses a state machine to track whether a resource is ready for use.
 * ResourceReference<T> instances query their state to determine if the underlying resource
 * has finished loading, is still pending, or encountered an error.
 *
 * State Transitions:
 *
 *     Unknown → Pending → Loaded
 *             ↘       ↘ Error
 *             ↘ Missing
 *             ↘ Null
 *
 * States are monotonic - once a reference reaches a terminal state (Loaded, Error, Missing, Null),
 * it never changes. The Pending state indicates active loading on the job system.
 *
 * Thread Safety:
 * State queries from ResourceReference are thread-safe. The registry uses internal locking to
 * ensure state transitions are atomic.
 *
 * Usage:
 * @code
 * auto texture_ref = registry.load<TextureResource>(STRING_ID("textures/albedo.png"));
 *
 * // Poll the state (non-blocking)
 * switch (texture_ref.get_state()) {
 *     case ResourceState::Loaded:
 *         // Resource ready to use
 *         auto& texture = texture_ref.get();
 *         break;
 *     case ResourceState::Pending:
 *         // Still loading, check again next frame
 *         break;
 *     case ResourceState::Error:
 *         // Load failed, handle error
 *         LOG_ERROR("Failed to load texture");
 *         break;
 *     case ResourceState::Missing:
 *         // Resource not found in database
 *         LOG_ERROR("Texture not found");
 *         break;
 * }
 * @endcode
 *
 * @note Resources themselves cannot have an invalid state - only references can be invalid.
 *       Once a resource is loaded into the registry, it's always valid.
 *
 * @see ResourceReference::get_state() for how references query their current state
 * @see ResourceRegistry::get_resource() for the underlying state lookup
 */
enum class ResourceState: uint8_t
{
    /**
     * @brief Initial state for references that haven't queried the registry yet
     *
     * When a ResourceReference is first created by load(), it starts in Unknown state.
     * The first call to get_state() or is_valid() will query the registry and transition
     * to the actual state (Pending, Loaded, Missing, or Error).
     *
     */
    Unknown = 0,

    /**
     * @brief Resource is fully loaded and ready for use
     *
     * Terminal state. The resource has been successfully loaded from disk/memory, processed
     * by the appropriate loader, and stored in the registry. Calling get() on a reference
     * in this state will return a valid resource pointer.
     *
     * @note This is the only state where get() is guaranteed to succeed without blocking.
     */
    Loaded  = 1,

    /**
     * @brief Resource was not found in the resource database
     *
     * Terminal state. The requested resource_id doesn't exist in the database's metadata.
     * This typically means the file doesn't exist in the scanned resource directories,
     * or the database hasn't been refreshed after adding new files.
     *
     */
    Missing = 2,

    /**
     * @brief Resource is currently being loaded on the job system
     *
     * Transient state. A background job is actively loading this resource. The job may be:
     * - Reading bytes from disk via ResourceSource
     * - Decoding the file format (PNG, GLTF, etc.)
     * - Uploading data to GPU memory
     * - Creating sub-resources for composite types
     *
     * References should check again later (typically next frame) to see if loading completed.
     *
     * @note For immediate_load(), this state is never observable since the call blocks.
     */
    Pending = 3,

    /**
     * @brief Resource loading failed due to an error
     *
     * Terminal state. The loader encountered an error during loading, such as:
     * - Corrupted file data
     * - Unsupported file format variant
     * - Out of memory
     * - GPU resource allocation failure
     *
     * Check application logs for detailed error messages from the loader.
     *
     */
    Error   = 4,

    Unloaded = 5,

    /**
     * @brief Special state for default-constructed or null references
     *
     * Terminal state. Indicates a reference created with INVALID_STRING_ID or moved-from.
     * Calling get() on a Null reference is undefined behavior - always check is_valid() first.
     *
     * @note This state is distinct from Missing - Null means the reference itself is invalid,
     *       while Missing means a valid reference couldn't find its resource.
     */
    Null
};

/**
 * @enum ResourceType
 * @brief Categorizes resources by their semantic type in the asset pipeline
 *
 * ResourceType identifies what kind of asset a resource represents. This determines:
 * - Which loader implementation handles the resource (via LoaderFactory)
 * - How the resource integrates with the renderer and ECS systems
 * - What metadata is associated with the resource in the database
 *
 * The type is extracted from the template parameter when calling ResourceRegistry::load<T>()
 * using the static method T::static_type(). For example, TextureResource::static_type()
 * returns ResourceType::Texture.
 *
 * Usage:
 * @code
 * // Type is inferred from template parameter
 * auto texture_ref = registry.load<TextureResource>(STRING_ID("textures/albedo.png"));
 * // Internally calls TextureResource::static_type() → ResourceType::Texture
 *
 * // LoaderFactory maps type to loader
 * auto& loader = loader_factory.get(ResourceType::Texture);
 * // Returns TextureLoader instance
 * @endcode
 *
 * @see LoaderFactory for the type-to-loader mapping
 * @see SourceMetadata::type for where this is stored in metadata
 * @see utils::find_extension_type() for file extension to type mapping
 */
enum class ResourceType: uint16_t
{
    Unknown   = 0,
    Material  = 1,
    Texture   = 2,
    Shader    = 3,
    Mesh      = 4,
    Scene     = 6,
    Composite = 7,
    Font      = 8
};

enum class SourceFormat: uint8_t
{
    Unknown,
    Memory,            // Source exists in memory
    Image,             // Image formats, e.g. PNG, JPEG
    Texture,           // Ktx or other texture formats
    Material,          // Material files, e.g. MDL
    Obj,               // Wavefront .obj files
    Shader,            // Shader files, e.g. slang
    PrecompiledShader, // Precompiled shader files, e.g. spv
    Glft,              // GLTF files
    FontFile,          // A font file, e.g. TTF
    Scene,             // A json scene file
    BinaryScene        // A binary scene file
};

namespace utils
{
    std::optional<std::pair<ResourceType, SourceFormat>> find_extension_type(std::string_view extension);
}
}

template <>
struct std::hash<portal::ResourceType>
{
    size_t operator()(const portal::ResourceType& type) const noexcept
    {
        return static_cast<uint16_t>(type);
    }
};
