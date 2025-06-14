//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>

#include "portal/core/common.h"
#include "portal/engine/strings/string_id.h"

namespace portal
{

enum class ResourceState: uint8_t
{
    Empty   = 0,      // The resource is not loaded, and has no data
    Loaded  = BIT(0), // The resource is loaded and ready to use
    Missing = BIT(1), // The resource was not found in the database
    Invalid = BIT(2), // The resource is not yet valid, e.g. not yet loaded, or uncounted an error during loading
};

enum class ResourceType: uint16_t
{
    Unknown,
    Texture
};

class Resource
{
public:
    const StringId id;
    explicit Resource(const StringId& id) : id(id), state(ResourceState::Invalid) {}

    virtual ~Resource() = default;

    [[nodiscard]] bool is_valid() const { return state == ResourceState::Loaded; }
    [[nodiscard]] ResourceState get_state() const { return state; }

private:
    friend class ResourceRegistry;

    ResourceState state;
};

}
