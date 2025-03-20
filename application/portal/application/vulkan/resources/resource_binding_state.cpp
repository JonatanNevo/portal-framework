//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "resource_binding_state.h"

namespace portal::vulkan
{
void ResourceSet::reset()
{
    clear_dirty();
    resource_bindings.clear();
}

bool ResourceSet::is_dirty() const
{
    return dirty;
}

void ResourceSet::clear_dirty()
{
    dirty = false;
}

void ResourceSet::clear_dirty(const uint32_t binding, const uint32_t array_element)
{
    resource_bindings[binding][array_element].dirty = false;
}

void ResourceSet::bind_buffer(
    const Buffer& buffer,
    const vk::DeviceSize offset,
    const vk::DeviceSize range,
    const unsigned int binding,
    const uint32_t array_element
)
{
    resource_bindings[binding][array_element].dirty = true;
    resource_bindings[binding][array_element].buffer = &buffer;
    resource_bindings[binding][array_element].offset = offset;
    resource_bindings[binding][array_element].range = range;
    dirty = true;
}

void ResourceSet::bind_image(const ImageView& image_view, const Sampler& sampler, const uint32_t binding, const uint32_t array_element)
{
    resource_bindings[binding][array_element].dirty = true;
    resource_bindings[binding][array_element].image_view = &image_view;
    resource_bindings[binding][array_element].sampler = &sampler;
    dirty = true;
}

void ResourceSet::bind_image(const ImageView& image_view, const uint32_t binding, const uint32_t array_element)
{
    resource_bindings[binding][array_element].dirty = true;
    resource_bindings[binding][array_element].image_view = &image_view;
    resource_bindings[binding][array_element].sampler = nullptr;
    dirty = true;
}

void ResourceSet::bind_input(const ImageView& image_view, const uint32_t binding, const uint32_t array_element)
{
    resource_bindings[binding][array_element].dirty = true;
    resource_bindings[binding][array_element].image_view = &image_view;
    dirty = true;
}

const BindingMap<ResourceInfo>& ResourceSet::get_resource_bindings() const
{
    return resource_bindings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void ResourceBindingState::reset()
{
    clear_dirty();
    resource_sets.clear();
}

bool ResourceBindingState::is_dirty() const
{
    return dirty;
}

void ResourceBindingState::clear_dirty()
{
    dirty = false;
}

void ResourceBindingState::clear_dirty(uint32_t set)
{
    resource_sets[set].clear_dirty();
}

void ResourceBindingState::bind_buffer(
    const Buffer& buffer,
    const vk::DeviceSize offset,
    const vk::DeviceSize range,
    const uint32_t set,
    const uint32_t binding,
    const uint32_t array_element
)
{
    resource_sets[set].bind_buffer(buffer, offset, range, binding, array_element);
    dirty = true;
}

void ResourceBindingState::bind_image(
    const ImageView& image_view,
    const Sampler& sampler,
    const uint32_t set,
    const uint32_t binding,
    const uint32_t array_element
)
{
    resource_sets[set].bind_image(image_view, sampler, binding, array_element);

    dirty = true;
}

void ResourceBindingState::bind_image(const ImageView& image_view, const uint32_t set, const uint32_t binding, const uint32_t array_element)
{
    resource_sets[set].bind_image(image_view, binding, array_element);

    dirty = true;
}

void ResourceBindingState::bind_input(const ImageView& image_view, const uint32_t set, const uint32_t binding, const uint32_t array_element)
{
    resource_sets[set].bind_input(image_view, binding, array_element);

    dirty = true;
}

const std::unordered_map<uint32_t, ResourceSet>& ResourceBindingState::get_resource_sets()
{
    return resource_sets;
}
} // portal
