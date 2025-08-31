//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "image.h"

namespace portal::renderer
{

size_t get_format_bytes_per_pixel(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::R8_UNorm:
    case ImageFormat::R8_UInt:
        return 1;
    case ImageFormat::R16_UInt:
    case ImageFormat::R16_Float:
    case ImageFormat::RG8_UNorm:
    case ImageFormat::RG8_UInt:
        return 2;
    case ImageFormat::RGB8_UNorm:
    case ImageFormat::RGB8_UInt:
    case ImageFormat::SRGB:
        return 3;
    case ImageFormat::R32_UInt:
    case ImageFormat::R32_Float:
    case ImageFormat::RG16_UInt:
    case ImageFormat::RG16_Float:
    case ImageFormat::RGBA8_UNorm:
    case ImageFormat::RGBA8_UInt:
    case ImageFormat::RG32_UInt:
    case ImageFormat::RG32_Float:
    case ImageFormat::SRGBA:
        return 4;
    case ImageFormat::RGB16_UInt:
    case ImageFormat::RGB16_Float:
        return 6;
    case ImageFormat::RGBA16_UInt:
    case ImageFormat::RGBA16_Float:
        return 8;
    case ImageFormat::RGB32_UInt:
    case ImageFormat::RGB32_Float:
        return 12;
    case ImageFormat::RGBA32_UInt:
    case ImageFormat::RGBA32_Float:
        return 16;
    default:
        break;
    }

    PORTAL_ASSERT(false, "Invalid format");
    return 0;
}

size_t utils::calculate_mip_count(const size_t width, const size_t height)
{
    return static_cast<size_t>(glm::floor(glm::log2(static_cast<float>(glm::min(width, height))))) + 1;
}

size_t utils::get_image_memory_size(const ImageFormat format, const size_t width, const size_t height)
{
    return get_format_bytes_per_pixel(format) * width * height;
}
} // portal