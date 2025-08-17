//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "texture_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <portal/core/debug/profile.h>

#include "portal/engine/renderer/allocated_buffer.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/resources/gpu_context.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/resources/resources/texture.h"

namespace portal::resources
{
static auto logger = Log::get_logger("Resources");

TextureLoader::TextureLoader(ResourceRegistry* registry, const std::shared_ptr<GpuContext>& context): ResourceLoader(registry), gpu_context(context) {}

void TextureLoader::initialize()
{
    const uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    const uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    const uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));

    constexpr vk::Extent3D default_extent{
        .width = 1,
        .height = 1,
        .depth = 1
    };

    std::array white_data {white};
    create_default_texture(Texture::WHITE_TEXTURE_ID, white_data, default_extent);

    std::array black_data {black};
    create_default_texture(Texture::BLACK_TEXTURE_ID, black_data, default_extent);

    std::array<uint32_t, 16 * 16> pixels{}; //for 16x16 checkerboard texture
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }

    constexpr vk::Extent3D missing_extent{
        .width = 16,
        .height = 16,
        .depth = 1
    };

    missing_texture = create_default_texture(Texture::MISSING_TEXTURE_ID, pixels, missing_extent);
}

bool TextureLoader::load(const std::shared_ptr<ResourceSource> source) const
{
    PORTAL_PROF_ZONE;
    auto texture = registry->get<Texture>(source->get_meta().source_id);
    if (!texture)
    {
        LOGGER_ERROR("Failed to load texture: resource is not a Texture: {}", texture->id);
        return false;
    }

    auto data = source->load();

    int width, height, n_channels;
    auto* image_data = stbi_load_from_memory(data.as<uint8_t*>(), static_cast<int>(data.size), &width, &height, &n_channels, 4);
    if (!image_data)
    {
        LOGGER_ERROR("Failed to load texture: {}", texture->id);
        return false;
    }

    const vk::Extent3D image_extent{
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .depth = 1
    };

    auto image = build_image_from_memory(texture->id, image_data, image_extent);

    const size_t data_size = image->get_extent().width * image->get_extent().height * image->get_extent().depth * 4;
    texture->image = std::move(image);
    texture->data = Buffer::copy(image_data, data_size);

    if (source->get_meta().format != SourceFormat::Memory)
    {
        constexpr vk::SamplerCreateInfo sampler_info{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
        };
        texture->sampler = std::make_shared<vk::raii::Sampler>(gpu_context->create_sampler(sampler_info));
    }
    // TODO: pass sampler info properly

    stbi_image_free(image_data);
    return true;
}

void TextureLoader::load_default(Ref<Resource>& resource) const
{
    if (missing_texture.is_valid())
    {
        resource.as<Texture>()->copy_from(missing_texture.lock());
    }
}

std::shared_ptr<vulkan::AllocatedImage> TextureLoader::build_image_from_memory(const StringId& id, void* data, const vk::Extent3D extent) const
{
    vulkan::ImageBuilder image_builder(extent);
    image_builder
        .with_format(vk::Format::eR8G8B8A8Unorm)
        .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc)
        .with_debug_name(std::string(id.string))
        .with_mips_levels(static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1);
    return gpu_context->create_image_shared(data, image_builder);
}

Ref<Texture> TextureLoader::create_default_texture(const StringId& id, const std::span<uint32_t> data, const vk::Extent3D extent) const
{
    auto texture = registry->get<Texture>(id);

    texture->data = Buffer::copy(data.data(), data.size() * sizeof(uint32_t));
    texture->image = build_image_from_memory(id, data.data(), extent);
    texture->set_state(ResourceState::Loaded);

    constexpr vk::SamplerCreateInfo sampler_info{
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
    };
    texture->sampler = std::make_shared<vk::raii::Sampler>(gpu_context->create_sampler(sampler_info));
    return texture;
}
} // portal
