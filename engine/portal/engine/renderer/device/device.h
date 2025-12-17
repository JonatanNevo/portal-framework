//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

namespace portal::renderer
{
class Device
{
public:
    virtual ~Device() = default;

    virtual void wait_idle() const = 0;
};
} // portal
