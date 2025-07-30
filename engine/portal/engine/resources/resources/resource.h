//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <utility>

#include "portal/core/buffer.h"
#include "portal/core/reference.h"
#include "portal/core/concurrency/spin_lock.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/strings/string_id.h"

#include "portal/engine/resources/resource_types.h"


namespace portal
{

namespace resources
{
    class ImageLoader;
    class ResourceLoader;
}

struct ResourceSignature
{
    StringId id;
    ResourceType type;
};

template <typename T>
class ResourceContainer
{
public:
    ResourceContainer(): resource(nullptr), data(nullptr) {}
    virtual ~ResourceContainer() = default;

    virtual void load(T&& new_resource, Buffer&& new_data)
    {
        data = std::move(new_data);
        resource = std::move(new_resource);
    }

    virtual void destroy()
    {
        data = nullptr;
        resource = nullptr;
    }

    [[nodiscard]] const T& get() const { return resource; }
    [[nodiscard]] const T* operator->() const { return &resource; }
    [[nodiscard]] const T& operator*() const { return resource; }

protected:
    T resource;
    Buffer data;
};

class Resource: public RefCounted
{
public:
    const StringId id;

    Resource(): id(INVALID_STRING_ID) {}
    explicit Resource(const StringId& id): id(id) {}

    [[nodiscard]] bool is_valid() const { return state == ResourceState::Loaded; }
    [[nodiscard]] ResourceState get_state() const { return state; }
    void set_state(const ResourceState new_state) { this->state = new_state; }

protected:
    ResourceState state = ResourceState::Empty;
};

class Texture final : public Resource, public ResourceContainer<vulkan::AllocatedImage>
{
public:
    Texture() = default;
    explicit Texture(const StringId& id): Resource(id) {}
};


}
