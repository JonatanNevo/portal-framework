//
// Created by Jonatan Nevo on 11/03/2025.
//

#pragma once
#include <unordered_map>

#include <portal/core/glm.h>

#include "portal/application/module/renderer/scene/component.h"

namespace portal::scene_graph
{
class Texture;

/**
 * @brief How the alpha value of the main factor and texture should be interpreted
 */
enum class AlphaMode
{
    /// Alpha value is ignored
    Opaque,
    /// Either full opaque or fully transparent
    Mask,
    /// Output is combined with the background
    Blend
};

class Material final : public Component
{
public:
    explicit Material(const std::string& name): Component(name) {}
    std::type_index get_type() override { return typeid(Material); }

    std::unordered_map<std::string, Texture*> textures;

    /// Emissive color of the material
    glm::vec3 emissive{0.0f, 0.0f, 0.0f};
    /// Whether the material is double sided
    bool double_sided{false};
    /// Cutoff threshold when in Mask mode
    float alpha_cutoff{0.5f};
    /// Alpha rendering mode
    AlphaMode alpha_mode{AlphaMode::Opaque};
};
} // portal
