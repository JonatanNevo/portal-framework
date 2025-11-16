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

struct DriverVersion
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
};

class PhysicalDevice
{
public:
    virtual ~PhysicalDevice() = default;

    /**
     * @return The vendor's driver version
     */
    [[nodiscard]] virtual DriverVersion get_driver_version() const = 0;

    /**
     * Checks if the device supports a specific extension by name
     */
    [[nodiscard]] virtual bool is_extension_supported(std::string_view extensions_name) const = 0;

    /**
     * Checks of the device is able to present to a given surface using a queue family
     * TODO: is this api agnostic?
     */
    [[nodiscard]] virtual bool supports_present(Surface& surface, uint32_t queue_family_index) const = 0;

    // TODO: should I add a api agnostic feature get/set?
};

} // portal
