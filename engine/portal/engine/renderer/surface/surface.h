//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "llvm/ADT/SmallVector.h"
#include "portal/core/strings/string_id.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/surface/capabilities.h"

namespace portal {
class Window;
}

namespace portal::renderer
{

struct SurfaceProperties
{
    StringId debug_name;

    size_t min_frames_in_flight = 3;
    // Passing `std::nullopt` for headless surface
    std::optional<std::reference_wrapper<Window>> window;
};

enum class SurfaceType
{
    Normal,
    Headless
};

class Surface
{
public:
    /**
     * Initializes the surface with the necessary properties.
     */
    explicit Surface(SurfaceProperties properties);

    virtual ~Surface() = default;

    /**
     * @brief Queries the capabilities of the surface for a specific physical device/adapter.
     *
     * @return A structured capability description (e.g., supported formats, presentation modes, extent).
     */
    [[nodiscard]] virtual const SurfaceCapabilities& get_capabilities() const = 0;

    /**
     * @brief Gets the current size of the renderable area in pixels.
     */
    [[nodiscard]] virtual glm::ivec2 get_extent() const = 0;

    [[nodiscard]] virtual SurfaceType get_type() const = 0;
    [[nodiscard]] virtual size_t get_min_frames_in_flight() const = 0;

protected:
    SurfaceProperties properties;
};

} // portal