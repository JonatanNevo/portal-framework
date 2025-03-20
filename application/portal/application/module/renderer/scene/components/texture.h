//
// Created by Jonatan Nevo on 11/03/2025.
//

#pragma once
#include "portal/application/module/renderer/scene/component.h"


namespace portal::scene_graph
{
class Sampler;
class Image;

class Texture final : public Component
{
public:
    explicit Texture(const std::string& name);
    std::type_index get_type() override;

    void set_image(Image& image);
    Image* get_image() const;

    void set_sampler(Sampler& sampler);
    Sampler* get_sampler() const;

private:
    Image* image = nullptr;
    Sampler* sampler = nullptr;
};
} // portal
