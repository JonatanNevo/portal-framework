//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "loader.h"

#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glm/gtx/quaternion.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/scene/gltf_scene.h"
#include "portal/engine/renderer/scene/scene_node.h"

#include <ranges>


namespace portal::vulkan
{

static auto logger = Log::get_logger("vulkan.loader");

vk::Filter extract_filter(const fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return vk::Filter::eNearest;
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapLinear:
    case fastgltf::Filter::LinearMipMapNearest:
        return vk::Filter::eLinear;
    }
    return vk::Filter::eLinear;
}

vk::SamplerMipmapMode extract_mipmap_mode(const fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return vk::SamplerMipmapMode::eNearest;
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
        return vk::SamplerMipmapMode::eLinear;
    }
    return vk::SamplerMipmapMode::eLinear;
}

std::optional<Image> load_image(vk::raii::Device& device, fastgltf::Asset& asset, fastgltf::Image& image, Renderer* renderer)
{
    Image new_image = nullptr;
    int width, height, n_channels;
    std::visit(
        fastgltf::visitor{
            [](std::monostate&)
            {
                LOG_ERROR_TAG("loaded", "Unsupported image source type 'monostate'");
            },
            [](fastgltf::sources::CustomBuffer&)
            {
                LOG_ERROR_TAG("loaded", "Unsupported image source type 'CustomBuffer'");
            },
            [](fastgltf::sources::ByteView&)
            {
                LOG_ERROR_TAG("loaded", "Unsupported image source type 'ByteView'");
            },
            [](fastgltf::sources::Fallback&)
            {
                LOG_ERROR_TAG("loaded", "Unsupported image source type 'Fallback'");
            },
            [&](const fastgltf::sources::URI& file_path)
            {
                assert(file_path.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(file_path.uri.isLocalPath());   // We're only capable of loading  local files.
                const std::string path(file_path.uri.path().begin(), file_path.uri.path().end());
                unsigned char* data = stbi_load(path.c_str(), &width, &height, &n_channels, 4);
                if (data)
                {
                    vk::Extent3D image_size;
                    image_size.width = width;
                    image_size.height = height;
                    image_size.depth = 1;

                    new_image = ImageBuilder(image_size)
                                .with_format(vk::Format::eR8G8B8A8Unorm)
                                .with_usage(
                                    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
                                    )
                                .with_debug_name(fmt::format("gltf_image_{}", image.name))
                                .with_mips_levels(static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1)
                                .build(device);
                    renderer->populate_image(data, new_image);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Array& arr)
            {
                unsigned char* data = stbi_load_from_memory(
                    reinterpret_cast<uint8_t*>(arr.bytes.data()),
                    static_cast<int>(arr.bytes.size()),
                    &width,
                    &height,
                    &n_channels,
                    4
                    );
                if (data)
                {
                    vk::Extent3D image_size;
                    image_size.width = width;
                    image_size.height = height;
                    image_size.depth = 1;

                    new_image = ImageBuilder(image_size)
                                .with_format(vk::Format::eR8G8B8A8Unorm)
                                .with_usage(
                                    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
                                    )
                                .with_debug_name(fmt::format("gltf_image_{}", image.name))
                                .with_mips_levels(static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1)
                                .build(device);
                    renderer->populate_image(data, new_image);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Vector& vec)
            {
                unsigned char* data = stbi_load_from_memory(
                    reinterpret_cast<uint8_t*>(vec.bytes.data()),
                    static_cast<int>(vec.bytes.size()),
                    &width,
                    &height,
                    &n_channels,
                    4
                    );
                if (data)
                {
                    vk::Extent3D image_size;
                    image_size.width = width;
                    image_size.height = height;
                    image_size.depth = 1;

                    new_image = ImageBuilder(image_size)
                                .with_format(vk::Format::eR8G8B8A8Unorm)
                                .with_usage(
                                    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
                                    )
                                .with_debug_name(fmt::format("gltf_image_{}", image.name))
                                .with_mips_levels(static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1)
                                .build(device);
                    renderer->populate_image(data, new_image);

                    stbi_image_free(data);
                }
            },
            [&](const fastgltf::sources::BufferView& view)
            {
                auto& buffer_view = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[buffer_view.bufferIndex];

                std::visit(
                    fastgltf::visitor{
                        [](std::monostate&) { LOG_ERROR_TAG("Vulkan", "Unsupported buffer_view image source type 'monostate'"); },
                        [](fastgltf::sources::BufferView&) { LOG_ERROR_TAG("Vulkan", "Unsupported buffer_view image source type 'BufferView'"); },
                        [](fastgltf::sources::URI&) { LOG_ERROR_TAG("Vulkan", "Unsupported buffer_view image source type 'URI'"); },
                        [](fastgltf::sources::CustomBuffer&) { LOG_ERROR_TAG("Vulkan", "Unsupported buffer_view image source type 'CustomBuffer'"); },
                        [](fastgltf::sources::ByteView&) { LOG_ERROR_TAG("Vulkan", "Unsupported buffer_view image source type 'ByteView'"); },
                        [](fastgltf::sources::Fallback&) { LOG_ERROR_TAG("Vulkan", "Unsupported buffer_view image source type 'Fallback'"); },
                        // We only care about VectorWithMime here, because we
                        // specify LoadExternalBuffers, meaning all buffers
                        // are already loaded into a vector.
                        [&](fastgltf::sources::Array& arr)
                        {
                            unsigned char* data = stbi_load_from_memory(
                                reinterpret_cast<uint8_t*>(arr.bytes.data() + buffer_view.byteOffset),
                                static_cast<int>(buffer_view.byteLength),
                                &width,
                                &height,
                                &n_channels,
                                4
                                );
                            if (data)
                            {
                                vk::Extent3D image_size;
                                image_size.width = width;
                                image_size.height = height;
                                image_size.depth = 1;

                                new_image = ImageBuilder(image_size)
                                            .with_format(vk::Format::eR8G8B8A8Unorm)
                                            .with_usage(
                                                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst |
                                                vk::ImageUsageFlagBits::eTransferSrc
                                                )
                                            .with_debug_name(fmt::format("gltf_image_{}", image.name))
                                            .with_mips_levels(static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1)
                                            .build(device);
                                renderer->populate_image(data, new_image);

                                stbi_image_free(data);
                            }
                        },
                        [&](fastgltf::sources::Vector& vector)
                        {
                            unsigned char* data = stbi_load_from_memory(
                                reinterpret_cast<uint8_t*>(vector.bytes.data() + buffer_view.byteOffset),
                                static_cast<int>(buffer_view.byteLength),
                                &width,
                                &height,
                                &n_channels,
                                4
                                );
                            if (data)
                            {
                                vk::Extent3D image_size;
                                image_size.width = width;
                                image_size.height = height;
                                image_size.depth = 1;

                                new_image = ImageBuilder(image_size)
                                            .with_format(vk::Format::eR8G8B8A8Unorm)
                                            .with_usage(
                                                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst |
                                                vk::ImageUsageFlagBits::eTransferSrc
                                                )
                                            .with_debug_name(fmt::format("gltf_image_{}", image.name))
                                            .with_mips_levels(static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1)
                                            .build(device);
                                renderer->populate_image(data, new_image);

                                stbi_image_free(data);
                            }
                        }
                    },
                    buffer.data
                    );
            }
        },
        image.data
        );

    // if any of the attempts to load the data failed, we have not written the image
    if (new_image.get_handle() == nullptr)
    {
        LOG_ERROR_TAG("Vulkan", "Failed to load image: {}", image.name);
        return std::nullopt;
    }
    return std::move(new_image);

}

std::optional<std::shared_ptr<GLTFScene>> load_gltf(vk::raii::Device& device, std::filesystem::path path, Renderer* renderer)
{
    LOGGER_INFO("Loading glTF scene from: {}", std::filesystem::absolute(path).string());
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (!data)
    {
        LOGGER_ERROR("Failed to load glTF meshes from: {}, error: {}",  std::filesystem::absolute(path).string(), fastgltf::getErrorName(data.error()));
        return std::nullopt;
    }

    auto scene = std::make_shared<GLTFScene>();
    scene->device = &device;

    constexpr auto glft_options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;

    fastgltf::Asset gltf;
    fastgltf::Parser parser;

    auto type = fastgltf::determineGltfFileType(data.get());
    switch (type)
    {
    case fastgltf::GltfType::glTF:
    {
        auto load = parser.loadGltf(data.get(), path.parent_path(), glft_options);
        if (load)
        {
            gltf = std::move(load.get());
            break;
        }
        LOGGER_ERROR("Failed to load glTF scene from: {}, error: {}",  std::filesystem::absolute(path).string(), fastgltf::getErrorName(load.error()));
        return std::nullopt;
    }
    case fastgltf::GltfType::GLB:
    {
        auto load = parser.loadGltfBinary(data.get(), path.parent_path(), glft_options);
        if (load)
        {
            gltf = std::move(load.get());
            break;
        }
        LOGGER_ERROR("Failed to load GLB scene from: {}, error: {}",  std::filesystem::absolute(path).string(), fastgltf::getErrorName(load.error()));
        return std::nullopt;
    }
    case fastgltf::GltfType::Invalid:
        LOGGER_ERROR("Failed to determine glTF file type for: {}",  std::filesystem::absolute(path).string());
        return std::nullopt;
    }

    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {vk::DescriptorType::eCombinedImageSampler, 3},
        {vk::DescriptorType::eUniformBuffer, 3},
        {vk::DescriptorType::eStorageBuffer, 1}
    };

    scene->descriptor_allocator.init(&device, static_cast<uint32_t>(gltf.materials.size()), sizes);

    // load samplers
    for (auto& sampler : gltf.samplers)
    {
        vk::SamplerCreateInfo sampler_info{
            .magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
            .minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .minLod = 0,
            .maxLod = vk::LodClampNone,
        };

        scene->samplers.emplace_back(device.createSampler(sampler_info));
    }

    // temporal arrays for all the objects to use while creating the GLTF data
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<SceneNode>> nodes;
    std::vector<Image*> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;

    // load all textures

    for ([[maybe_unused]] auto& image : gltf.images)
    {
        std::optional<Image> img = load_image(device, gltf, image, renderer);
        if (img.has_value())
        {
            scene->images[image.name.c_str()] = std::move(img.value());
            images.push_back(&scene->images[image.name.c_str()]);
        }
        else
        {
            // we failed to load, so lets give the slot a default error texture to not completely break loading
            images.push_back(&renderer->error_checker_board_image);
        }
    }

    scene->material_data = BufferBuilder(sizeof(GLTFMetallicRoughness::MaterialConsts) * gltf.materials.size())
                           .with_usage(vk::BufferUsageFlagBits::eUniformBuffer)
                           .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                           .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                           .with_debug_name(fmt::format("gltf_material_data_{}", path.stem().string()))
                           .build(device);

    auto* material_consts = static_cast<GLTFMetallicRoughness::MaterialConsts*>(scene->material_data.get_data());

    uint32_t data_index = 0;
    for (auto& material : gltf.materials)
    {
        auto new_material = std::make_shared<GLTFMaterial>();
        materials.push_back(new_material);
        scene->materials[material.name.c_str()] = new_material;

        GLTFMetallicRoughness::MaterialConsts consts{
            .color_factors = {
                material.pbrData.baseColorFactor[0],
                material.pbrData.baseColorFactor[1],
                material.pbrData.baseColorFactor[2],
                material.pbrData.baseColorFactor[3]
            },
            .metal_rough_factors = {
                material.pbrData.metallicFactor,
                material.pbrData.roughnessFactor,
                0.f,
                0.f
            }
        };

        material_consts[data_index] = consts;

        auto pass = MaterialPass::MainColor;
        if (material.alphaMode == fastgltf::AlphaMode::Blend)
        {
            pass = MaterialPass::Transparent;
        }

        uint32_t offset = data_index * sizeof(GLTFMetallicRoughness::MaterialConsts);
        GLTFMetallicRoughness::MaterialResources material_resources{
            .color_image = &renderer->white_image,
            .color_sampler = &renderer->default_sampler_linear,
            .metallic_roughness_image = &renderer->white_image,
            .metallic_roughness_sampler = &renderer->default_sampler_linear,
            .data_buffer = &scene->material_data,
            .data_buffer_offset = offset
        };

        // grab textures from gltf file
        if (material.pbrData.baseColorTexture.has_value())
        {
            size_t image = gltf.textures[material.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[material.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            material_resources.color_image = images[image];
            material_resources.color_sampler = &scene->samplers[sampler];
        }

        new_material->data = renderer->metal_rough_material.write_material(
            device,
            pass,
            material_resources,
            scene->descriptor_allocator
            );

        data_index++;
    }

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;


    for (auto& mesh : gltf.meshes)
    {
        auto new_mesh = std::make_shared<MeshAsset>();
        meshes.push_back(new_mesh);
        scene->meshes[mesh.name.c_str()] = new_mesh;
        new_mesh->name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives)
        {
            GeoSurface new_surface{};
            new_surface.start_index = static_cast<uint32_t>(indices.size());
            new_surface.count = static_cast<uint32_t>(gltf.accessors[p.indicesAccessor.value()].count);

            uint32_t initial_vertex = static_cast<uint32_t>(vertices.size());

            // load indexes
            {
                auto& index_accessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + index_accessor.count);

                fastgltf::iterateAccessor<uint32_t>(
                    gltf,
                    index_accessor,
                    [&](const uint32_t& index)
                    {
                        indices.push_back(index + initial_vertex);
                    }
                    );
            }

            // load vertex positions
            {
                auto& position_accessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + position_accessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf,
                    position_accessor,
                    [&](glm::vec3 v, size_t index)
                    {
                        vertices[initial_vertex + index] = {
                            .position = v,
                            .uv_x = 0,
                            .normal = {1, 0, 0},
                            .uv_y = 0,
                            .color = glm::vec4{1.f},
                        };
                    }
                    );
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf,
                    gltf.accessors[normals->accessorIndex],
                    [&](glm::vec3 v, size_t index)
                    {
                        vertices[initial_vertex + index].normal = v;
                    }
                    );
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    gltf,
                    gltf.accessors[uv->accessorIndex],
                    [&](glm::vec2 v, size_t index)
                    {
                        vertices[initial_vertex + index].uv_x = v.x;
                        vertices[initial_vertex + index].uv_y = v.y;
                    }
                    );
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    gltf,
                    gltf.accessors[colors->accessorIndex],
                    [&](glm::vec4 v, size_t index)
                    {
                        vertices[initial_vertex + index].color = v;
                    }
                    );
            }

            if (p.materialIndex.has_value())
            {
                new_surface.material = materials[p.materialIndex.value()];
            }
            else
            {
                new_surface.material = materials[0];
            }

            //loop the vertices of this surface, find min/max bounds
            glm::vec3 min_pos = vertices[initial_vertex].position;
            glm::vec3 max_pos = vertices[initial_vertex].position;
            for (size_t i = initial_vertex; i < vertices.size(); ++i)
            {
                min_pos = glm::min(min_pos, vertices[i].position);
                max_pos = glm::max(max_pos, vertices[i].position);
            }

            // calculate origin and extents from the min/max, use extent lenght for radius
            new_surface.bounds.origin = (max_pos + min_pos) / 2.f;
            new_surface.bounds.extents = (max_pos - min_pos) / 2.f;
            new_surface.bounds.sphere_radius = glm::length(new_surface.bounds.extents);

            new_mesh->surfaces.push_back(new_surface);\
        }

        new_mesh->vertices = vertices;
        new_mesh->indices = indices;
    }

    // load all nodes and their meshes
    for (auto& node : gltf.nodes)
    {
        std::shared_ptr<SceneNode> new_node;

        // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the mesh node class
        if (node.meshIndex.has_value())
        {
            new_node = std::make_shared<MeshNode>();
            dynamic_cast<MeshNode*>(new_node.get())->mesh = meshes[node.meshIndex.value()];
        }
        else
        {
            new_node = std::make_shared<SceneNode>();
        }

        nodes.push_back(new_node);
        scene->nodes[node.name.c_str()] = new_node;

        std::visit(
            fastgltf::visitor{[&](fastgltf::math::fmat4x4 matrix)
                              {
                                  std::memcpy(&new_node->local_transform, matrix.data(), sizeof(matrix));
                              },
                              [&](fastgltf::TRS transform)
                              {
                                  glm::vec3 tl(
                                      transform.translation[0],
                                      transform.translation[1],
                                      transform.translation[2]
                                      );
                                  glm::quat rot(
                                      transform.rotation[3],
                                      transform.rotation[0],
                                      transform.rotation[1],
                                      transform.rotation[2]
                                      );
                                  glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                  glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                  glm::mat4 rm = glm::toMat4(rot);
                                  glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                  new_node->local_transform = tm * rm * sm;
                              }},
            node.transform
            );
    }

    // run loop again to setup transform hierarchy
    for (size_t i = 0; i < gltf.nodes.size(); i++)
    {
        fastgltf::Node& node = gltf.nodes[i];
        auto& scene_node = nodes[i];

        for (auto& c : node.children)
        {
            scene_node->children.push_back(nodes[c]);
            nodes[c]->parent = scene_node;
        }
    }

    // find the top nodes, with no parents
    for (auto& node : nodes)
    {
        if (node->parent.lock() == nullptr)
        {
            scene->top_nodes.push_back(node);
            node->refresh_transform(glm::mat4{1.f});
        }
    }

    return scene;
}
}
