//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <cstdint>
#include <string>

namespace portal::renderer
{
class Surface;

/**
 * @struct DriverVersion
 * @brief GPU driver version
 *
 * Semantic version (major.minor.patch) for driver identification.
 */
struct DriverVersion
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
};

/**
 * @class PhysicalDevice
 * @brief Abstract physical GPU device
 *
 * Queries device capabilities, extensions, and surface presentation support.
 */
class PhysicalDevice
{
public:
    virtual ~PhysicalDevice() = default;

    /**
     * @brief Gets driver version
     * @return Vendor driver version
     */
    [[nodiscard]] virtual DriverVersion get_driver_version() const = 0;

    /**
     * @brief Checks extension support
     * @param extensions_name Extension name
     * @return True if extension is supported
     */
    [[nodiscard]] virtual bool is_extension_supported(std::string_view extensions_name) const = 0;

    /**
     * @brief Checks presentation support
     * @param surface Target surface
     * @param queue_family_index Queue family to check
     * @return True if queue family can present to surface
     */
    [[nodiscard]] virtual bool supports_present(Surface& surface, uint32_t queue_family_index) const = 0;
};
} // portal
