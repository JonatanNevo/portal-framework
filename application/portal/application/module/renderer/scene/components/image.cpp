//
// Created by Jonatan Nevo on 11/03/2025.
//

#include "image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <portal/core/file_system.h>

namespace portal::scene_graph
{
bool is_astc(const vk::Format format)
{
    return (format == vk::Format::eAstc4x4UnormBlock ||
        format == vk::Format::eAstc4x4SrgbBlock ||
        format == vk::Format::eAstc5x4UnormBlock ||
        format == vk::Format::eAstc5x4SrgbBlock ||
        format == vk::Format::eAstc5x5UnormBlock ||
        format == vk::Format::eAstc5x5SrgbBlock ||
        format == vk::Format::eAstc6x5UnormBlock ||
        format == vk::Format::eAstc6x5SrgbBlock ||
        format == vk::Format::eAstc6x6UnormBlock ||
        format == vk::Format::eAstc6x6SrgbBlock ||
        format == vk::Format::eAstc8x5UnormBlock ||
        format == vk::Format::eAstc8x5SrgbBlock ||
        format == vk::Format::eAstc8x6UnormBlock ||
        format == vk::Format::eAstc8x6SrgbBlock ||
        format == vk::Format::eAstc8x8UnormBlock ||
        format == vk::Format::eAstc8x8SrgbBlock ||
        format == vk::Format::eAstc10x5UnormBlock ||
        format == vk::Format::eAstc10x5SrgbBlock ||
        format == vk::Format::eAstc10x6UnormBlock ||
        format == vk::Format::eAstc10x6SrgbBlock ||
        format == vk::Format::eAstc10x8UnormBlock ||
        format == vk::Format::eAstc10x8SrgbBlock ||
        format == vk::Format::eAstc10x10UnormBlock ||
        format == vk::Format::eAstc10x10SrgbBlock ||
        format == vk::Format::eAstc12x10UnormBlock ||
        format == vk::Format::eAstc12x10SrgbBlock ||
        format == vk::Format::eAstc12x12UnormBlock ||
        format == vk::Format::eAstc12x12SrgbBlock);
}

// When the color-space of a loaded image is unknown (from KTX1 for example) we
// may want to assume that the loaded data is in sRGB format (since it usually is).
// In those cases, this helper will get called which will force an existing unorm
// format to become a srgb format where one exists. If none exist, the format will
// remain unmodified.
static vk::Format maybe_coerce_to_srgb(const vk::Format fmt)
{
    switch (fmt)
    {
    case vk::Format::eR8Unorm:
        return vk::Format::eR8Srgb;
    case vk::Format::eR8G8Unorm:
        return vk::Format::eR8G8Srgb;
    case vk::Format::eR8G8B8Unorm:
        return vk::Format::eR8G8B8Srgb;
    case vk::Format::eB8G8R8Unorm:
        return vk::Format::eB8G8R8Srgb;
    case vk::Format::eR8G8B8A8Unorm:
        return vk::Format::eR8G8B8A8Srgb;
    case vk::Format::eB8G8R8A8Unorm:
        return vk::Format::eB8G8R8A8Srgb;
    case vk::Format::eA8B8G8R8UnormPack32:
        return vk::Format::eA8B8G8R8SrgbPack32;
    case vk::Format::eBc1RgbUnormBlock:
        return vk::Format::eBc1RgbSrgbBlock;
    case vk::Format::eBc1RgbaUnormBlock:
        return vk::Format::eBc1RgbaSrgbBlock;
    case vk::Format::eBc2UnormBlock:
        return vk::Format::eBc2SrgbBlock;
    case vk::Format::eBc3UnormBlock:
        return vk::Format::eBc3SrgbBlock;
    case vk::Format::eBc7UnormBlock:
        return vk::Format::eBc7SrgbBlock;
    case vk::Format::eEtc2R8G8B8UnormBlock:
        return vk::Format::eEtc2R8G8B8SrgbBlock;
    case vk::Format::eEtc2R8G8B8A1UnormBlock:
        return vk::Format::eEtc2R8G8B8A1SrgbBlock;
    case vk::Format::eEtc2R8G8B8A8UnormBlock:
        return vk::Format::eEtc2R8G8B8A8SrgbBlock;
    case vk::Format::eAstc4x4UnormBlock:
        return vk::Format::eAstc4x4SrgbBlock;
    case vk::Format::eAstc5x4UnormBlock:
        return vk::Format::eAstc5x4SrgbBlock;
    case vk::Format::eAstc5x5UnormBlock:
        return vk::Format::eAstc5x5SrgbBlock;
    case vk::Format::eAstc6x5UnormBlock:
        return vk::Format::eAstc6x5SrgbBlock;
    case vk::Format::eAstc6x6UnormBlock:
        return vk::Format::eAstc6x6SrgbBlock;
    case vk::Format::eAstc8x5UnormBlock:
        return vk::Format::eAstc8x5SrgbBlock;
    case vk::Format::eAstc8x6UnormBlock:
        return vk::Format::eAstc8x6SrgbBlock;
    case vk::Format::eAstc8x8UnormBlock:
        return vk::Format::eAstc8x8SrgbBlock;
    case vk::Format::eAstc10x5UnormBlock:
        return vk::Format::eAstc10x5SrgbBlock;
    case vk::Format::eAstc10x6UnormBlock:
        return vk::Format::eAstc10x6SrgbBlock;
    case vk::Format::eAstc10x8UnormBlock:
        return vk::Format::eAstc10x8SrgbBlock;
    case vk::Format::eAstc10x10UnormBlock:
        return vk::Format::eAstc10x10SrgbBlock;
    case vk::Format::eAstc12x10UnormBlock:
        return vk::Format::eAstc12x10SrgbBlock;
    case vk::Format::eAstc12x12UnormBlock:
        return vk::Format::eAstc12x12SrgbBlock;
    case vk::Format::ePvrtc12BppUnormBlockIMG:
        return vk::Format::ePvrtc12BppSrgbBlockIMG;
    case vk::Format::ePvrtc14BppUnormBlockIMG:
        return vk::Format::ePvrtc14BppSrgbBlockIMG;
    case vk::Format::ePvrtc22BppUnormBlockIMG:
        return vk::Format::ePvrtc22BppSrgbBlockIMG;
    case vk::Format::ePvrtc24BppUnormBlockIMG:
        return vk::Format::ePvrtc24BppSrgbBlockIMG;
    default:
        return fmt;
    }
}

// Note that this function returns the required size for ALL mip levels, *including* the base level.
uint32_t get_required_mipmaps_size(const VkExtent3D& extent)
{
    constexpr uint32_t channels = 4;
    auto width = std::max<uint32_t>(1, extent.width);
    auto height = std::max<uint32_t>(1, extent.height);
    auto size = width * height * channels;
    auto result = size;
    while (size != channels)
    {
        width = std::max<uint32_t>(1u, width >> 1);
        height = std::max<uint32_t>(1u, height >> 1);
        size = width * height * channels;
        result += size;
    }
    return result;
}

Image::Image(const std::string& name, std::vector<uint8_t>&& data, std::vector<Mipmap>&& mipmaps): Component(name),
                                                                                                   data(std::move(data)),
                                                                                                   format(vk::Format::eR8G8B8A8Unorm),
                                                                                                   mipmaps(std::move(mipmaps))
{}

std::unique_ptr<Image> Image::load(const std::string& name, const std::string& uri, ContentType content_type)
{
    std::unique_ptr<Image> image;
    auto data = filesystem::read_file_binary(uri);
    const auto extension = filesystem::get_file_extension(uri);

    if (extension == "png" || extension == "jpg")
    {
        image = std::make_unique<Stb>(name, data, content_type);
    }
    else if (extension == "astc")
    {
        image = std::make_unique<Astc>(name, data);
    }
    else if (extension == "ktx")
    {
        image = std::make_unique<Ktx>(name, data, content_type);
    }
    else if (extension == "ktx2")
    {
        image = std::make_unique<Ktx>(name, data, content_type);
    }

    return image;
}

std::type_index Image::get_type()
{
    return typeid(Image);
}

const std::vector<uint8_t>& Image::get_data() const
{
    return data;
}

void Image::clear_data()
{
    data.clear();
    data.shrink_to_fit();
}

vk::Format Image::get_format() const
{
    return format;
}

const vk::Extent3D& Image::get_extent() const
{
    PORTAL_CORE_ASSERT(!mipmaps.empty(), "No mipmaps");
    return mipmaps[0].extent;
}

uint32_t Image::get_layers() const
{
    return layers;
}

const std::vector<Mipmap>& Image::get_mipmaps() const
{
    return mipmaps;
}

const std::vector<std::vector<vk::DeviceSize>>& Image::get_offsets() const
{
    return offsets;
}

void Image::generate_mipmaps()
{
    PORTAL_CORE_ASSERT(mipmaps.size() == 1, "Mipmaps already generated");

    if (mipmaps.size() > 1)
        return;

    auto extent = get_extent();
    auto next_width = std::max<uint32_t>(1, extent.width / 2);
    auto next_height = std::max<uint32_t>(1, extent.height / 2);
    auto channels = 4;
    auto next_size = next_width * next_height * channels;

    // Allocate for all the mips at once.  The function returns the total size needed for the
    // existing base mip as well as all the mips that will be generated.
    data.reserve(get_required_mipmaps_size(extent));

    while (!(next_width == 1 && next_height == 1))
    {
        // Make space for next mipmap
        uint32_t old_size = data.size();
        data.resize(old_size + next_size);

        auto& prev_mipmap = mipmaps.back();
        // Update mipmaps
        Mipmap next_mipmap{
            .level = prev_mipmap.level + 1,
            .offset = old_size,
            .extent = {next_width, next_height, 1}
        };

        // Fill next mipmap memory
        stbir_resize_uint8_linear(
            data.data() + prev_mipmap.offset,
            prev_mipmap.extent.width,
            prev_mipmap.extent.height,
            0,
            data.data() + next_mipmap.offset,
            next_mipmap.extent.width,
            next_mipmap.extent.height,
            0,
            static_cast<stbir_pixel_layout>(channels)
        );

        mipmaps.emplace_back(std::move(next_mipmap));
        // Next mipmap values
        next_width = std::max<uint32_t>(1, next_width / 2);
        next_height = std::max<uint32_t>(1, next_height / 2);
        next_size = next_width * next_height * channels;
    }
}

void Image::create_vk_image(vulkan::Device& device, vk::ImageViewType image_view_type, vk::ImageCreateFlags flags)
{
    PORTAL_CORE_ASSERT(!vk_image && !vk_image_view, "Image already created");
    vk_image = vulkan::ImageBuilder(get_extent())
               .with_format(format)
               .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
               .with_vma_usage(vma::MemoryUsage::eGpuOnly)
               .with_sample_count(vk::SampleCountFlagBits::e1)
               .with_mips_levels(mipmaps.size())
               .with_array_layers(layers)
               .with_tiling(vk::ImageTiling::eOptimal)
               .with_flags(flags)
               .build_unique(device);
    vk_image->set_debug_name(get_name());

    vk_image_view = std::make_unique<vulkan::ImageView>(*vk_image, image_view_type);
    vk_image_view->set_debug_name(std::format("View of {}", get_name()));
}

const vulkan::Image& Image::get_vk_image() const
{
    return *vk_image;
}

const vulkan::ImageView& Image::get_vk_image_view() const
{
    return *vk_image_view;
}

void Image::coerce_format_to_srgb()
{
    format = maybe_coerce_to_srgb(format);
}

std::vector<uint8_t>& Image::get_mut_data()
{
    return data;
}

void Image::set_data(const uint8_t* raw_data, const size_t size)
{
    data = {raw_data, raw_data + size};
}

void Image::set_format(const vk::Format format)
{
    this->format = format;
}

void Image::set_width(const uint32_t width)
{
    mipmaps[0].extent.width = width;
}

void Image::set_height(const uint32_t height)
{
    mipmaps[0].extent.height = height;
}

void Image::set_depth(const uint32_t depth)
{
    mipmaps[0].extent.depth = depth;
}

void Image::set_layers(const uint32_t layers)
{
    this->layers = layers;
}

void Image::set_offsets(const std::vector<std::vector<vk::DeviceSize>>& offsets)
{
    this->offsets = offsets;
}

Mipmap& Image::get_mipmap(const size_t index)
{
    return mipmaps[index];
}

std::vector<Mipmap>& Image::get_mut_mipmaps()
{
    return mipmaps;
}
}
