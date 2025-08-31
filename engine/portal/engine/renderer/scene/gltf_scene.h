//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_raii.hpp>

#include "renderable.h"
#include "../vulkan/allocated_buffer.h"
#include "portal/engine/renderer/descriptor_allocator.h"

namespace portal
{
struct SceneNode;

namespace vulkan {
    struct MeshAsset;
    struct GLTFMaterial;
    class Image;
}

struct GLTFScene final : public Renderable
{
public:
    std::unordered_map<std::string, std::shared_ptr<vulkan::MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<SceneNode>> nodes;
    std::unordered_map<std::string, renderer::vulkan::Image> images;
    std::unordered_map<std::string, std::shared_ptr<vulkan::GLTFMaterial>> materials;

    std::vector<std::shared_ptr<SceneNode>> top_nodes;
    std::vector<vk::raii::Sampler> samplers;
    vulkan::DescriptorAllocator descriptor_allocator;
    renderer::vulkan::AllocatedBuffer material_data;

    vk::raii::Device* device;

    ~GLTFScene() override;
    void draw(const glm::mat4& top_matrix, DrawContext& context) override;
private:
    void clear_all();
};

} // portal
