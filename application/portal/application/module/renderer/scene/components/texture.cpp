//
// Created by Jonatan Nevo on 11/03/2025.
//

#include "texture.h"

namespace portal::scene_graph
{
Texture::Texture(const std::string& name): Component(name)
{}

std::type_index Texture::get_type()
{
    return typeid(Texture);
}

void Texture::set_image(Image& image)
{
    this->image = &image;
}

Image* Texture::get_image() const
{
    return image;
}

void Texture::set_sampler(Sampler& sampler)
{
    this->sampler = &sampler;
}

Sampler* Texture::get_sampler() const
{
    return sampler;
}
} // portal
