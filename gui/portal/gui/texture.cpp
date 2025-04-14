//
// Created by thejo on 4/11/2025.
//

#include "texture.h"

#include <stb_image.h>

#include "portal/core/file_system.h"

namespace portal
{

// TODO: Load this dynamically / create it dynamically
static std::shared_ptr<Image> g_default_image = nullptr;

Texture::Texture(const TextureSpecification spec):
    specification(spec)
{
    if (!g_default_image)
    {
        const auto image_data = portal::FileSystem::read_file_binary("resources/textures/missing_texture.png");
        size_t width, height;
        auto data = Image::decode(image_data.data, image_data.size, width, height);
        g_default_image = std::make_shared<Image>(width, height, ImageFormat::RGBA, data);
    }
}

std::shared_ptr<Image>& Texture::get_image()
{
    if (state != AssetState::Loaded)
    {
        return g_default_image;
    }
    return image;
}

void Texture::set_data(Buffer new_data)
{
    size_t width, height;
    data = new_data;
    void* decoded_data = portal::Image::decode(
        new_data.data,
        new_data.size,
        width,
        height
        );

    if (decoded_data)
    {
        // Create Vulkan image from decoded data
        image = std::make_shared<portal::Image>(
            width,
            height,
            portal::ImageFormat::RGBA,
            decoded_data
            );

        // Free the decoded data since it's now copied to the Vulkan image
        stbi_image_free(decoded_data);
    }
    else
    {
        LOG_ERROR("Failed to decode image data");
        image = g_default_image;
    }
}

} // portal
