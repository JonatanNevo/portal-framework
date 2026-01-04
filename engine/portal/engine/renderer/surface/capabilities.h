//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/glm.h>

#include "portal/core/flags.h"

namespace portal::renderer
{
/**
 * @enum SurfaceTransformBits
 * @brief Surface transformation flags
 *
 * Pre-transform operations for display rotation and mirroring.
 */
enum class SurfaceTransformBits: uint16_t
{
    Emtpy           = 0b000000000,  ///< No transform
    Identity        = 0b000000001,  ///< No rotation
    Rotate90        = 0b000000010,  ///< 90° clockwise
    Rotate180       = 0b000000100,  ///< 180° rotation
    Rotate270       = 0b000001000,  ///< 270° clockwise
    Mirror          = 0b000010000,  ///< Horizontal flip
    MirrorRotate90  = 0b000100000,  ///< Mirror + 90° rotation
    MirrorRotate180 = 0b001000000,  ///< Mirror + 180° rotation
    MirrorRotate270 = 0b010000000,  ///< Mirror + 270° rotation
    Inherit         = 0b100000000   ///< Use current transform
};

using SurfaceTransform = Flags<SurfaceTransformBits>;

/**
 * @struct SurfaceCapabilities
 * @brief Surface limits and supported features
 *
 * Swapchain image count, extent limits, layers, and transform support.
 */
struct SurfaceCapabilities
{
    size_t min_swapchain_images;
    size_t max_swapchain_images;

    glm::ivec2 current_extent;
    glm::ivec2 min_image_extent;
    glm::ivec2 max_image_extent;

    size_t max_image_array_layers;

    SurfaceTransform supported_transforms;
    SurfaceTransform current_transform;
};
}


template <>
struct portal::FlagTraits<portal::renderer::SurfaceTransformBits>
{
    using enum renderer::SurfaceTransformBits;

    static constexpr bool is_bitmask = true;
    static constexpr auto all_flags = Identity | Rotate90 | Rotate180 | Rotate270 | Mirror | MirrorRotate90 | MirrorRotate180 | MirrorRotate270 |
        Inherit;
};
