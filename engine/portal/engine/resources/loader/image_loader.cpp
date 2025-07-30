//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <portal/core/debug/profile.h>

#include "portal/engine/renderer/allocated_buffer.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/renderer/vulkan_utils.h"
#include "portal/engine/resources/gpu_context.h"
#include "portal/engine/resources/source/resource_source.h"

namespace portal::resources
{
static auto logger = Log::get_logger("Resources");

ImageLoader::ImageLoader(const std::shared_ptr<GpuContext>& runner): gpu_context(runner)
{
    const uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    const uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));

    std::array<uint32_t, 16 * 16> pixels{}; //for 16x16 checkerboard texture
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    default_texture_data = Buffer(pixels.data(), pixels.size() * sizeof(uint32_t));
}

void ImageLoader::init(std::shared_ptr<ResourceSource> resource_source)
{
    source = std::move(resource_source);
}

bool ImageLoader::load(const Ref<Resource>& resource)
{
    PORTAL_PROF_ZONE;
    auto texture_resource = resource.as<Texture>();
    if (!texture_resource)
    {
        LOGGER_ERROR("Failed to load texture: resource is not a Texture: {}", resource->id);
        return false;
    }

    auto data = source->load();

    int width, height, n_channels;
    auto* image_data = stbi_load_from_memory(data.as<uint8_t*>(), static_cast<int>(data.size), &width, &height, &n_channels, 4);
    if (!image_data)
    {
        LOGGER_ERROR("Failed to load texture: {}", resource->id);
        return false;
    }

    const vk::Extent3D image_extent{
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .depth = 1
    };

    vulkan::ImageBuilder image_builder(image_extent);
    image_builder
        .with_format(vk::Format::eR8G8B8A8Unorm)
        .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc)
        .with_debug_name(std::string(resource->id.string))
        .with_mips_levels(static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1);
    auto image = gpu_context->create_image(image_data, image_builder);

    const size_t data_size = image.get_extent().width * image.get_extent().height * image.get_extent().depth * 4;
    texture_resource->load(std::move(image), {image_data, data_size});

    stbi_image_free(image_data);
    return true;
}

void ImageLoader::load_default(const Ref<Resource>& resource) const
{
    auto texture_resource = resource.as<Texture>();
    if (!texture_resource)
    {
        LOGGER_ERROR("Failed to load default texture: resource is not a Texture: {}", resource->id);
        return;
    }

    // Create a reference to the default texture data
    texture_resource->load(create_default_texture(), {default_texture_data, default_texture_data.size});
    texture_resource->set_state(ResourceState::Invalid);
}

std::vector<ResourceSignature> ImageLoader::get_signature() const
{
    return {};
}

vulkan::AllocatedImage ImageLoader::create_default_texture() const
{
    const auto image_builder = vulkan::ImageBuilder(16, 16, 1)
                         .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc)
                         .with_format(vk::Format::eR8G8B8A8Unorm)
                         .with_tiling(vk::ImageTiling::eOptimal)
                         .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);
    return gpu_context->create_image(
        default_texture_data.as<uint8_t*>(),
        image_builder
        );
}
} // portal
