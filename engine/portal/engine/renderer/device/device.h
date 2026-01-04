//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal::renderer
{
/**
 * @class Device
 * @brief Abstract GPU device interface
 *
 * Provides device-level synchronization operations.
 */
class Device
{
public:
    virtual ~Device() = default;

    /** @brief Waits for all device operations to complete */
    virtual void wait_idle() const = 0;
};
} // portal
