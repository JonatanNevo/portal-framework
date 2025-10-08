//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/reference.h"
#include "portal/engine/strings/string_id.h"

#include "portal/engine/resources/resource_types.h"

namespace portal
{

namespace resources
{
    class TextureLoader;
    class ResourceLoader;
}

struct ResourceSignature
{
    StringId id;
    ResourceType type = ResourceType::Unknown;
};

class Resource: public RefCounted
{
public:
    const StringId id;

    Resource(): id(INVALID_STRING_ID) {}
    explicit Resource(const StringId& id): id(id) {}

    virtual void copy_from(Ref<Resource>) { PORTAL_ASSERT(false, "All resources must implement `copy_from`");}

    [[nodiscard]] bool is_valid() const { return state == ResourceState::Loaded; }
    [[nodiscard]] ResourceState get_state() const { return state; }
    void set_state(const ResourceState new_state) { this->state = new_state; }
protected:
    ResourceState state = ResourceState::Unknown;
};

}
