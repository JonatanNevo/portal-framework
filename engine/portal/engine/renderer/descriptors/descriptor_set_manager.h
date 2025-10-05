//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/reference.h"
#include "portal/engine/strings/string_id.h"
#include "portal/engine/renderer/descriptors/descriptor_input.h"

namespace portal::renderer
{
class ShaderVariant;

struct DescriptorSetManagerSpecification
{
    Ref<ShaderVariant> shader;
    StringId debug_name;

    // Set range to manage
    size_t start_set = 0;
    size_t end_set = std::numeric_limits<size_t>::max();

    Ref<renderer::Texture> default_texture;

    // TODO: get from global config
    size_t frame_in_flights;
};

class DescriptorSetManager
{
public:
    virtual ~DescriptorSetManager() = default;

    virtual void set_input(StringId name, Ref<UniformBufferSet> buffer) = 0;
    virtual void set_input(StringId name, Ref<UniformBuffer> buffer) = 0;
    virtual void set_input(StringId name, Ref<StorageBufferSet> buffer) = 0;
    virtual void set_input(StringId name, Ref<StorageBuffer> buffer) = 0;
    virtual void set_input(StringId name, Ref<Texture> texture) = 0;
    virtual void set_input(StringId name, Ref<Image> image) = 0;
    virtual void set_input(StringId name, Ref<ImageView> image) = 0;

    template<typename T>
    Ref<T> get_input(const StringId name)
    {
        return get_input(name).as<T>();
    }

    virtual Ref<RefCounted> get_input(StringId name) = 0;

    virtual bool is_invalidated(size_t set, size_t binding_index) const = 0;
    virtual bool validate() = 0;
    virtual void bake() = 0;
};

}
