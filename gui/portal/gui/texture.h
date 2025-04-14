//
// Created by thejo on 4/11/2025.
//

#pragma once
#include "image.h"
#include "portal/assets/asset.h"

namespace portal
{

struct TextureSpecification
{
    ImageFormat format = ImageFormat::RGBA;
    uint32_t width = 1;
    uint32_t height = 1;
};

class Texture final : public Asset
{
public:
    Texture(TextureSpecification spec);

    [[nodiscard]] AssetType get_type() const override { return AssetType::Texture; }
    [[nodiscard]] ImageFormat get_format() const { return specification.format; }
    [[nodiscard]] uint32_t get_width() const { return specification.width; }
    [[nodiscard]] uint32_t get_height() const { return specification.height; }
    [[nodiscard]] glm::uvec2 get_size() const { return {specification.width, specification.height}; }

    std::shared_ptr<Image>& get_image();

protected:
    void set_data(Buffer new_data) override;

private:
    TextureSpecification specification;

    std::shared_ptr<Image> image;
};

} // portal
