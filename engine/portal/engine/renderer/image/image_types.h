//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <limits>

#include <portal/core/glm.h>
#include "portal/core/strings/string_id.h"

namespace portal::renderer
{
class Image;

/**
 * @enum ImageFormat
 * @brief Pixel formats for images and render targets
 *
 * Supports normalized, integer, and floating-point formats in R/RG/RGB/RGBA configurations,
 * plus depth/stencil formats for render targets.
 */
enum class ImageFormat
{
    None,
    // R
    R8_UNorm,

    R8_UInt,
    R16_UInt,
    R32_UInt,

    R16_Float,
    R32_Float,

    // RG
    RG8_UNorm,

    RG8_UInt,
    RG16_UInt,
    RG32_UInt,

    RG16_Float,
    RG32_Float,

    // RGB
    RGB8_UNorm,
    BGRA8_UNorm,

    RGB8_UInt,
    RGB16_UInt,
    RGB32_UInt,

    RGB16_Float,
    RGB32_Float,

    // RGBA
    RGBA8_UNorm,

    RGBA8_UInt,
    RGBA16_UInt,
    RGBA32_UInt,

    RGBA16_Float,
    RGBA32_Float,

    SRGB,
    SRGBA,

    // Depth

    Depth_32Float_Stencil_8UInt,
    Depth_32Float,
    Depth_24UNorm_Stencil_8UInt,
    Depth_16UNorm_Stencil_8UInt,
    Depth_16UNorm,

    // default
    Depth = Depth_32Float_Stencil_8UInt
};

/**
 * @enum ImageUsage
 * @brief Image usage modes
 */
enum class ImageUsage
{
    None,
    Texture,
    Storage,
    Attachment,
    HostRead
};

/**
 * @enum TextureWrap
 * @brief Texture coordinate wrapping modes
 */
enum class TextureWrap
{
    None,
    Clamp,
    Repeat
};

/**
 * @enum TextureFilter
 * @brief Texture sampling filter modes
 */
enum class TextureFilter
{
    None,
    Linear,
    Nearest,
    Cubic
};

/**
 * @enum SamplerMipmapMode
 * @brief Mipmap sampling modes
 */
enum class SamplerMipmapMode
{
    None,
    Linear,
    Nearest
};

/**
 * @enum TextureType
 * @brief Texture dimensionality types
 */
enum class TextureType
{
    None,
    Texture,
    TextureCube
};

/**
 * @struct SamplerProperties
 * @brief Sampler configuration
 */
struct SamplerProperties
{
    TextureWrap wrap = TextureWrap::Repeat;
    TextureFilter filter = TextureFilter::Nearest;

    SamplerMipmapMode mipmap_mode = SamplerMipmapMode::Nearest;
    float min_lod = 0.f;
    float max_lod = 1000.f;
};

namespace image
{
    /**
     * @enum Flags
     * @brief Image creation flags
     */
    enum class Flags
    {
        None,
        CubeCompatible
    };

    /**
     * @struct Properties
     * @brief Image creation parameters
     */
    struct Properties
    {
        ImageFormat format = ImageFormat::RGBA8_UNorm;
        ImageUsage usage = ImageUsage::Texture;
        Flags flags = Flags::None;

        // Will this image be used for transfer operations
        bool transfer = false;
        size_t width = 1;
        size_t height = 1;
        size_t depth = 1;

        size_t mips = 1;
        size_t layers = 1;
        bool create_sampler = true;

        StringId name;
    };

    /**
     * @struct SubresourceRange
     * @brief Image subresource selection (mip/layer ranges)
     */
    struct SubresourceRange
    {
        size_t base_mip = 0;
        size_t mip_count = std::numeric_limits<size_t>::max();
        size_t base_layer = 0;
        size_t layer_count = std::numeric_limits<size_t>::max();
    };

    /**
     * @struct ClearValue
     * @brief Clear values for different image formats
     */
    struct ClearValue
    {
        glm::vec4 float_values;
        glm::ivec4 int_values;
        glm::uvec4 uint_values;
    };
}

/**
 * @struct TextureProperties
 * @brief Texture creation parameters with mipmap and storage options
 */
struct TextureProperties
{
    ImageFormat format = ImageFormat::RGBA8_UNorm;
    TextureType type = TextureType::Texture;
    size_t width = 1;
    size_t height = 1;
    size_t depth = 1;

    std::optional<SamplerProperties> sampler_prop = std::nullopt;

    bool generate_mipmaps = true;
    bool storage = false;
    bool store_locally = false;
};

namespace utils
{
    /**
     * @brief Checks if format is a depth format
     * @param format Image format to check
     * @return True if format contains depth component
     */
    inline bool is_depth_format(const ImageFormat format)
    {
        return format == ImageFormat::Depth_32Float || format == ImageFormat::Depth_32Float_Stencil_8UInt || format ==
            ImageFormat::Depth_24UNorm_Stencil_8UInt || format == ImageFormat::Depth_16UNorm_Stencil_8UInt || format == ImageFormat::Depth_16UNorm;
    }

    /**
     * @brief Checks if format is a stencil format
     * @param format Image format to check
     * @return True if format contains stencil component
     */
    inline bool is_stencil_format(const ImageFormat format)
    {
        return format == ImageFormat::Depth_32Float_Stencil_8UInt || format == ImageFormat::Depth_24UNorm_Stencil_8UInt || format ==
            ImageFormat::Depth_16UNorm_Stencil_8UInt;
    }

    /**
     * @brief Checks if format is an integer format
     * @param format Image format to check
     * @return True if format is integer (not normalized or floating-point)
     */
    inline bool is_integer_format(const ImageFormat format)
    {
        switch (format)
        {
        case ImageFormat::R8_UInt:
        case ImageFormat::R16_UInt:
        case ImageFormat::R32_UInt:
        case ImageFormat::RG8_UInt:
        case ImageFormat::RG16_UInt:
        case ImageFormat::RG32_UInt:
        case ImageFormat::RGB8_UInt:
        case ImageFormat::RGB16_UInt:
        case ImageFormat::RGB32_UInt:
        case ImageFormat::RGBA8_UInt:
        case ImageFormat::RGBA16_UInt:
        case ImageFormat::RGBA32_UInt:
            return true;

        case ImageFormat::R8_UNorm:
        case ImageFormat::R16_Float:
        case ImageFormat::R32_Float:
        case ImageFormat::RG8_UNorm:
        case ImageFormat::RG16_Float:
        case ImageFormat::RG32_Float:
        case ImageFormat::RGB8_UNorm:
        case ImageFormat::RGB16_Float:
        case ImageFormat::RGB32_Float:
        case ImageFormat::RGBA8_UNorm:
        case ImageFormat::SRGB:
        case ImageFormat::SRGBA:
        case ImageFormat::Depth_32Float_Stencil_8UInt:
        case ImageFormat::Depth_24UNorm_Stencil_8UInt:
        case ImageFormat::Depth_16UNorm_Stencil_8UInt:
        case ImageFormat::Depth_16UNorm:
        case ImageFormat::RGBA16_Float:
        case ImageFormat::RGBA32_Float:
        case ImageFormat::Depth_32Float:
        default:
            return false;
        }
    }
}
}
