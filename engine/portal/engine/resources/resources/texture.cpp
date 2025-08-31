//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "texture.h"

namespace portal
{
const StringId Texture::MISSING_TEXTURE_ID = STRING_ID("missing_texture");
const StringId Texture::WHITE_TEXTURE_ID = STRING_ID("white_texture");
const StringId Texture::BLACK_TEXTURE_ID = STRING_ID("black_texture");

void Texture::copy_from(const Ref<Resource> other)
{
    auto other_texture = other.as<Texture>();
    data = other_texture->data;
    image = other_texture->image;
    sampler = other_texture->sampler;
}

renderer::vulkan::VulkanImage& Texture::get()
{
    return *image;
}

const renderer::vulkan::VulkanImage& Texture::get() const
{
    return *image;
}

vk::raii::Sampler& Texture::get_sampler()
{
    return *sampler;
}

const vk::raii::Sampler& Texture::get_sampler() const
{
    return *sampler;
}
} // portal
