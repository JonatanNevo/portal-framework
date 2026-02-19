//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "texture_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "portal/core/log.h"
#include "portal/engine/renderer/renderer_context.h"

#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/resources/source/resource_source.h"
#include "portal/engine/renderer/image/image.h"

namespace portal::resources
{
static auto logger = Log::get_logger("Resources");


TextureLoader::TextureLoader(ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context) : ResourceLoader(registry), context(context)
{
    const uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    const uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    const uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));

    constexpr vk::Extent3D default_extent{
        .width = 1,
        .height = 1,
        .depth = 1
    };

    std::array white_data{white};
    create_standalone_texture(renderer::Texture::WHITE_TEXTURE_ID, white_data, default_extent);

    std::array black_data{black};
    create_standalone_texture(renderer::Texture::BLACK_TEXTURE_ID, black_data, default_extent);

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

    create_standalone_texture(renderer::Texture::MISSING_TEXTURE_ID, pixels, missing_extent);

    std::array<uint32_t, 6> black_cube_texture_data = {0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000};
    create_standalone_texture(renderer::Texture::BLACK_CUBE_TEXTURE_ID, black_cube_texture_data, default_extent, renderer::TextureType::TextureCube);
}

ResourceData TextureLoader::load(const SourceMetadata& meta, Reference<ResourceSource> source)
{
    auto data = source->load();
    int width, height, n_channels;
    void* image_data = nullptr;
    renderer::ImageFormat format;

    if (stbi_is_hdr_from_memory(data.as<uint8_t*>(), static_cast<int>(data.size)))
    {
        image_data = stbi_loadf_from_memory(data.as<const stbi_uc*>(), static_cast<int>(data.size), &width, &height, &n_channels, STBI_rgb_alpha);
        format = renderer::ImageFormat::RGBA32_Float;
    }
    else
    {
        image_data = stbi_load_from_memory(data.as<const stbi_uc*>(), static_cast<int>(data.size), &width, &height, &n_channels, STBI_rgb_alpha);
        format = renderer::ImageFormat::RGBA8_UNorm;
    }

    const size_t size = renderer::utils::get_image_memory_size(format, width, height, 1);

    if (!image_data)
    {
        LOGGER_ERROR("Failed to load texture: {}", meta.resource_id);
        stbi_image_free(image_data);
        return ResourceData{};
    }

    renderer::TextureProperties properties = {
        .format = format,
        .width = static_cast<size_t>(width),
        .height = static_cast<size_t>(height),
        .depth = 1
    };

    if (meta.format != SourceFormat::Memory)
    {
        properties.sampler_prop = {
            .filter = renderer::TextureFilter::Linear
        };
    }
    // TODO: get sampler info from metadata

    auto texture = make_reference<renderer::vulkan::VulkanTexture>(meta.resource_id, properties, Buffer{image_data, size}, context);

    stbi_image_free(image_data);
    return ResourceData{texture, source, meta};
}

void TextureLoader::enrich_metadata(SourceMetadata& meta, const ResourceSource& source)
{
    const auto data = source.load();
    int width, height, n_channels;
    void* image_data = nullptr;
    renderer::ImageFormat format;

    if (stbi_is_hdr_from_memory(data.as<uint8_t*>(), static_cast<int>(data.size)))
    {
        image_data = stbi_loadf_from_memory(data.as<const stbi_uc*>(), static_cast<int>(data.size), &width, &height, &n_channels, STBI_rgb_alpha);
        format = renderer::ImageFormat::RGBA32_Float;
    }
    else
    {
        image_data = stbi_load_from_memory(data.as<const stbi_uc*>(), static_cast<int>(data.size), &width, &height, &n_channels, STBI_rgb_alpha);
        format = renderer::ImageFormat::RGBA8_UNorm;
    }

    meta.meta = TextureMetadata{
        .hdr = static_cast<bool>(stbi_is_hdr_from_memory(data.as<uint8_t*>(), static_cast<int>(data.size))),
        .width = static_cast<size_t>(width),
        .height = static_cast<size_t>(height),
        .format = format
    };

    stbi_image_free(image_data);
}

void TextureLoader::save(ResourceData&) {}

void TextureLoader::create_standalone_texture(
    const StringId& id,
    const std::span<uint32_t> data,
    const vk::Extent3D extent,
    const renderer::TextureType type
) const
{
    const renderer::TextureProperties properties = {
        .format = renderer::ImageFormat::RGBA8_UNorm,
        .type = type,
        .width = static_cast<size_t>(extent.width),
        .height = static_cast<size_t>(extent.height),
        .depth = static_cast<size_t>(extent.depth),
        .sampler_prop = renderer::SamplerProperties{
            .filter = renderer::TextureFilter::Nearest,
        }
    };

    registry.allocate<renderer::vulkan::VulkanTexture>(
        id,
        id,
        properties,
        Buffer{data.data(), data.size() * sizeof(uint32_t)},
        context
    );
}
} // portal
